/*
Multicrew
Copyright (C) 2004,2005 Stefan Schimanski

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning (disable : 4786)

#define _WIN32_DCOM 
#include <objbase.h>

#include <list>
#include <deque>
#include <map>
#include <set>
#include <vector>
#include <sstream>

#include "../stlplus/source/string_utilities.hpp"

#include "streams.h"
#include "signals.h"
#include "multicrewcore.h"
#include "position.h"
#include "log.h"
#include "config.h"
#include "fsuipcmodule.h"


enum {
	fullSendPacket=0,
	channelPacket
};


typedef TypedPacket<char, PacketBase> MulticrewPacket;
typedef EmptyPacket FullSendPacket;


class ChannelPacket : public PacketBase {
public:
	ChannelPacket( SmartPtr<PacketBase> packet, std::string channel ) {
		this->_packet = packet;
		this->_channel = channel;
	}	

	virtual unsigned compiledSize() {
		return _channel.length()+1+_packet->compiledSize();
	}

	virtual void compile( void *data ) {
		strcpy( (char*)data, _channel.c_str() );
		_packet->compile( ((char*)data)+_channel.length()+1 );
	}

	std::string channel() {
		return _channel;
	}

	SmartPtr<PacketBase> packet() {
		return _packet;
	}

private:	
	std::string _channel;
	SmartPtr<PacketBase> _packet;
};


/*********************************************************************/


class MulticrewCoreImpl : public MulticrewCore {
 public:
	MulticrewCoreImpl();
	virtual ~MulticrewCoreImpl();
	void start();

	Mode mode();
	void setMode( Mode newMode );
	void useConnection( SmartPtr<Connection> con );
	void sendFullState();
	void handleFullState();
	unsigned registerNetworkChannel( NetworkChannel *channel );
	void unregisterNetworkChannel( NetworkChannel *channel );
	void receiveChannelPacket( SharedBuffer &buffer );
	void receive( void *data, unsigned length );
	void log( std::string line );
	double time();
	void callbackAsync();
	void triggerAsyncCallback( AsyncCallee *callee );
	void ackNewFrame();
	void registerModule( MulticrewModule *module );
	void unregisterModule( MulticrewModule *module );   
	bool sendChannelPacket( SmartPtr<PacketBase> packet, bool safe,
							Priority prio, unsigned channel );
	struct Data;
	Data *d;
};


struct MulticrewCoreImpl::Data {	
	/* mode handling */
	Mode mode;

	/* modules */
	std::map<std::string, MulticrewModule*> modules;
	SmartPtr<PositionModule> posModule;
	SmartPtr<FsuipcModule> fsuipcModule;

	/* network stuff */
	SmartPtr<Connection> connection;
	std::set<AsyncCallee*> asyncCallees;
	std::map<std::string, NetworkChannel*> channels; 

	/* timing */
	__int64 perfTimerFreq;
	__int64 startTime;

	CRITICAL_SECTION critSect;
};


static MulticrewCoreImpl *multicrewCore = 0;

SmartPtr<MulticrewCore> MulticrewCore::multicrewCore() {
	if( ::multicrewCore==0 ) ::multicrewCore = new MulticrewCoreImpl();

	// dout << "MulticrewCore::multicrewCore()" << std::endl;
	return ::multicrewCore;
}


/*********************************************************************/


struct NetworkChannel::Data {
	SmartPtr<MulticrewCoreImpl> core;
	std::string id;
	unsigned num;
};


NetworkChannel::NetworkChannel( std::string id ) {
	d = new Data;
	d->core = (MulticrewCoreImpl*)&*MulticrewCore::multicrewCore();
	d->id = id;
	d->num = d->core->registerNetworkChannel( this );
}


NetworkChannel::~NetworkChannel() {
	d->core->unregisterNetworkChannel( this );
	delete d;
}


std::string NetworkChannel::channelId() {
	return d->id;
}


unsigned NetworkChannel::channelNum() {
	return d->num;
}


bool NetworkChannel::send( SmartPtr<PacketBase> packet, bool safe, 
						   Priority prio ) {
	return d->core->sendChannelPacket(
		new ChannelPacket( packet, d->id ), 
		safe, prio, d->num );
}


/*********************************************************************/


MulticrewCoreImpl::MulticrewCoreImpl() {
	d = new Data;
	d->mode = IdleMode;
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	dout << "MulticrewCore" << std::endl;
	InitializeCriticalSection( &d->critSect );

	/* initialize timing stuff */
	if( !QueryPerformanceFrequency((LARGE_INTEGER*)&d->perfTimerFreq) )
		dlog << "No performance timer available" << std::endl;
	QueryPerformanceCounter( (LARGE_INTEGER*)&d->startTime );
}


void MulticrewCoreImpl::start() {
	/* create general modules */
	d->posModule = new PositionModule();
	d->fsuipcModule = new FsuipcModule();
}


MulticrewCoreImpl::~MulticrewCoreImpl() {
	dout << "~MulticrewCore" << std::endl;
	d->modules.clear();
	::multicrewCore = 0;
	CoUninitialize();

	DeleteCriticalSection( &d->critSect );
	delete d;
}


double MulticrewCoreImpl::time() {
	__int64 now;
	QueryPerformanceCounter( (LARGE_INTEGER*)&now );
	return ((double)now-d->startTime)/d->perfTimerFreq;
}


void MulticrewCoreImpl::ackNewFrame() {
	frameSignal.emit();
}


void MulticrewCoreImpl::registerModule( MulticrewModule *module ) {
	dout << "module " << module->id() << " registered" << std::endl;

	// register module
	d->modules[module->id().c_str()] = module;	

	// setup fsuipc watches
	if( !d->fsuipcModule.isNull() ) {
		SmartPtr<FileConfig> config = module->config();
		for( int i=1; ; i++ ) {
			std::ostringstream num;
			num << i;
			
			// get fsuipc line
			std::string line = config->stringValue( "fsuipc", num.str(), "" );
			if( line.length()==0 ) break;
			
			// break line into components
			std::vector<std::string> tokens =
               split( line, "," );
			if( tokens.size()!=3 )
				dlog << "invalid fsuipc for " << module->id 
					 << " id " << num << std::endl;
			
			// convert into datatypes		
			int id = (int)to_uint( trim(tokens[0]) );
			int len = (int)to_uint( trim(tokens[1]) );
			bool safe = to_bool( trim(tokens[2]) );
			dout << "fsuipc watch " << id << " len=" << len 
				 << " safe=" << safe << std::endl;
			
			// add to watches
			d->fsuipcModule->watch( id, len, safe );
		}
	}
}


void MulticrewCoreImpl::unregisterModule( MulticrewModule *module ) {
	dout << "module " << module->id() << " unregistered" << std::endl;

	// find and remove module from module list
	std::map<std::string, MulticrewModule*>::iterator res = d->modules.find( module->id().c_str() );
	if( res!=d->modules.end() ) {
		d->modules.erase( res );
	}
}


MulticrewCore::Mode MulticrewCoreImpl::mode() {
	return d->mode;
}


void MulticrewCoreImpl::setMode( Mode newMode ) {
	if( newMode!=d->mode ) {
		d->mode = newMode;
		modeChanged.emit( d->mode );
	}
}


void MulticrewCoreImpl::log( std::string line ) {
	logged.emit( line.c_str() );
}


void MulticrewCoreImpl::useConnection( SmartPtr<Connection> con ) {
	EnterCriticalSection( &d->critSect );
	d->connection = con;
	LeaveCriticalSection( &d->critSect );

	// finally start fsuipc and position module
	if( d->posModule.isNull() ) start();
}


void MulticrewCoreImpl::triggerAsyncCallback( AsyncCallee *callee ) {
	EnterCriticalSection( &d->critSect );
	bool first = d->asyncCallees.size()==0;

	// add callee to list of callback requesters
	d->asyncCallees.insert( callee );

	// let async callback handler know about the new callback
	if( first ) initAsyncCallback.emit();
	LeaveCriticalSection( &d->critSect );
}


void MulticrewCoreImpl::callbackAsync() {
	EnterCriticalSection( &d->critSect );
	for( std::set<AsyncCallee*>::iterator it = d->asyncCallees.begin();
		 it!=d->asyncCallees.end();
		 it++ ) {
		// do callback
		(*it)->asyncCallback();
	}
	d->asyncCallees.clear();
	LeaveCriticalSection( &d->critSect );
}


unsigned MulticrewCoreImpl::registerNetworkChannel( NetworkChannel *channel ) {
	EnterCriticalSection( &d->critSect );
	d->channels[channel->channelId()] = channel;
	unsigned ret = d->channels.size();
	LeaveCriticalSection( &d->critSect );
	return ret;
}


void MulticrewCoreImpl::unregisterNetworkChannel( NetworkChannel *channel ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannel *>::iterator result =
		d->channels.find( channel->channelId() );
	if( result!=d->channels.end() ) 
		d->channels.erase( result );
	LeaveCriticalSection( &d->critSect );
}


bool MulticrewCoreImpl::sendChannelPacket( SmartPtr<PacketBase> packet, 
										   bool safe, Priority prio, 
										   unsigned channel ) {
	bool ret = false;
	EnterCriticalSection( &d->critSect );
	if( !d->connection.isNull() ) {
		ret = d->connection->send( 
			new MulticrewPacket( channelPacket, packet ),
			safe, prio, channel );
	}
	LeaveCriticalSection( &d->critSect );
	return ret;
}


void MulticrewCoreImpl::handleFullState() {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannel *>::iterator it = d->channels.end();
	while( it!=d->channels.end() ) {
		it->second->sendFullState();
		it++;
	}
	LeaveCriticalSection( &d->critSect );
}


void MulticrewCoreImpl::sendFullState() {
	switch( mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
		handleFullState();
		break;
	case MulticrewCore::ClientMode: {		
		SmartPtr<PacketBase> packet = 
			new MulticrewPacket( 
				fullSendPacket, 
				new FullSendPacket );
		d->connection->send( packet, true, highPriority, 0 );
	} break;
	}
}


void MulticrewCoreImpl::receive( void *data, unsigned length ) {
	SharedBuffer buffer( data, length );
	switch( *(char*)buffer.data() ) {
	case fullSendPacket:
		handleFullState();
		break;
	case channelPacket:
		receiveChannelPacket( SharedBuffer( buffer, 1 ) );
		break;
	}
}


void MulticrewCoreImpl::receiveChannelPacket( SharedBuffer &buffer ) {
	EnterCriticalSection( &d->critSect );
	std::string moduleId = (char*)buffer.data();

	// find destination
	std::map<std::string,NetworkChannel *>::iterator channel =
		d->channels.find( moduleId );
	if( channel==d->channels.end() ) {
		dout << "Unroutable packet for module \"" << moduleId << "\"" << std::endl;
	} else {
		// create packet and send it to receiver object
		SmartPtr<PacketBase> packet = 
			channel->second->createPacket( 
				SharedBuffer( buffer, moduleId.length() ) );
		channel->second->receive( packet );
	}

	LeaveCriticalSection( &d->critSect );
}


/*********************************************************************************/


void AsyncCallee::triggerAsyncCallback() {
	MulticrewCore::multicrewCore()->triggerAsyncCallback( this );
}
