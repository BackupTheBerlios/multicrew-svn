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


class ChannelPacket : public StringTypedPacket<PacketBase> {
 public:
	ChannelPacket( std::string channel, SmartPtr<PacketBase> packet ) 
		: StringTypedPacket<PacketBase>( channel, packet ) {
	}	

	ChannelPacket( SharedBuffer &buffer, Factory *factory ) 
		: StringTypedPacket<PacketBase>( buffer, factory ) {
	}

	std::string channel() {
		return key();
	}
};


typedef std::set<NetworkChannel*> NetworkChannelSet;


/*********************************************************************/


class MulticrewCoreImpl : public MulticrewCore, 
						  private TypedPacketFactory<char,PacketBase>,
						  private StringTypedPacketFactory<PacketBase> {
 public:
	MulticrewCoreImpl();
	virtual ~MulticrewCoreImpl();
	void start();

	Mode mode();
	void start( bool host, SmartPtr<Connection> con );
	void stop();
	void sendFullState();
	void handleFullState();
	unsigned registerNetworkChannel( NetworkChannel *channel );
	void unregisterNetworkChannel( NetworkChannel *channel );
	void receive( SmartPtr<PacketBase> packet );
	SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );
	SmartPtr<PacketBase> createPacket( char key, SharedBuffer &buffer );
	SmartPtr<PacketBase> createPacket( std::string channel, SharedBuffer &buffer );
	void log( std::string line );
	double time();
	void callbackAsync();
	void triggerAsyncCallback( AsyncCallee *callee );
	void ackNewFrame();
	void registerModule( MulticrewModule *module );
	void unregisterModule( MulticrewModule *module );   
	bool sendChannelPacket( SmartPtr<ChannelPacket> packet, bool safe,
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
	std::map<std::string, NetworkChannelSet> channels; 

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
		new ChannelPacket( d->id, packet ), 
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


void MulticrewCoreImpl::start( bool host, SmartPtr<Connection> con ) {
	EnterCriticalSection( &d->critSect );
	d->mode = host?HostMode:ClientMode;
	d->connection = con;
	LeaveCriticalSection( &d->critSect );

	// startup procedure
	if( d->posModule.isNull() ) start();
	if( d->fsuipcModule.isNull() ) start();
	sendFullState();
	modeChanged.emit( d->mode );
}


void MulticrewCoreImpl::stop() {
	EnterCriticalSection( &d->critSect );
	d->mode = IdleMode;
	d->connection = 0;
	LeaveCriticalSection( &d->critSect );

	modeChanged.emit( d->mode );
}


void MulticrewCoreImpl::log( std::string line ) {
	logged.emit( line.c_str() );
}


void MulticrewCoreImpl::triggerAsyncCallback( AsyncCallee *callee ) {
	EnterCriticalSection( &d->critSect );
	bool first = d->asyncCallees.size()==0;
	//dout << "triggering async (first" << first << ")" << std::endl;

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
	std::map<std::string, NetworkChannelSet>::iterator it =
		d->channels.find( channel->channelId() );
	if( it==d->channels.end() )
		d->channels[channel->channelId()] = NetworkChannelSet();
	d->channels[channel->channelId()].insert( channel );
	unsigned ret = d->channels.size();
	dout << "Network channel " << channel->channelId() << " max=" << ret << std::endl;
	LeaveCriticalSection( &d->critSect );
	return ret;
}


void MulticrewCoreImpl::unregisterNetworkChannel( NetworkChannel *channel ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannelSet>::iterator it =
		d->channels.find( channel->channelId() );
	if( it!=d->channels.end() ) 
		d->channels[channel->channelId()].erase( channel );
	LeaveCriticalSection( &d->critSect );
}


bool MulticrewCoreImpl::sendChannelPacket( SmartPtr<ChannelPacket> packet, 
										   bool safe, Priority prio, 
										   unsigned channel ) {
	bool ret = false;
	EnterCriticalSection( &d->critSect );
	SmartPtr<Connection> con = d->connection;
	LeaveCriticalSection( &d->critSect );

	if( !con.isNull() ) {
		return con->send( 
			new MulticrewPacket( channelPacket, &*packet ),
			safe, prio, channel );
	}
	return false;
}


void MulticrewCoreImpl::handleFullState() {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannelSet>::iterator it = d->channels.begin();
	while( it!=d->channels.end() ) {
		NetworkChannelSet::iterator it2 = it->second.begin();
		while( it2!=it->second.end() ) {
			(*it2)->sendFullState();
			it2++;
		}
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


SmartPtr<PacketBase> MulticrewCoreImpl::createPacket( SharedBuffer &buffer ) {
	return new MulticrewPacket( buffer, this );
}


SmartPtr<PacketBase> MulticrewCoreImpl::createPacket( char key, 
													  SharedBuffer &buffer ) {
	switch( key ) {
	case fullSendPacket:
		return new FullSendPacket();
		break;
	case channelPacket:
		return new ChannelPacket( buffer, this );
		break;
	}
	return 0;
}

SmartPtr<PacketBase> MulticrewCoreImpl::createPacket( std::string channel,
													  SharedBuffer &buffer ) {
    // find destination
	std::map<std::string,NetworkChannelSet>::iterator dest =
		d->channels.find( channel );
	if( dest==d->channels.end() ) {
		dout << "Unroutable packet for module \"" << channel
			 << "\"" << std::endl;
	} else {
		std::set<NetworkChannel*>::iterator it = dest->second.begin();
		if( it!=dest->second.end() )
			return (*it)->createPacket( buffer );
		else
			dout << "Unroutable packet for module \"" << channel
				 << "\"" << std::endl;
	}
	return 0;	
}


void MulticrewCoreImpl::receive( SmartPtr<PacketBase> packet ) {
	SmartPtr<MulticrewPacket> mp = (MulticrewPacket*)&*packet;
	switch( mp->key() ) {
	case fullSendPacket:
		handleFullState();
		break;
	case channelPacket: {
		EnterCriticalSection( &d->critSect );
		SmartPtr<ChannelPacket> ncp = (ChannelPacket*)&*mp->wrappee();
		
        // find destination
		std::map<std::string,NetworkChannelSet>::iterator dest =
			d->channels.find( ncp->key() );
		if( dest==d->channels.end() ) {
			dout << "Unroutable packet for module \"" << ncp->key() 
				 << "\"" << std::endl;
			LeaveCriticalSection( &d->critSect );
		} else {			
			LeaveCriticalSection( &d->critSect );
			std::set<NetworkChannel*>::iterator it = dest->second.begin();
			while( it!=dest->second.end() ) {
				(*it)->receive( ncp->wrappee() );			
				it++;
			}
		}		
	}
	}
}


/*********************************************************************************/


void AsyncCallee::triggerAsyncCallback() {
	//dout << "triggering async" << std::endl;
	MulticrewCore::multicrewCore()->triggerAsyncCallback( this );
}
