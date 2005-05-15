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

#include "multicrewcore.h"
#include "networkimpl.h"
#include "streams.h"
#include "log.h"



/************************************ packets ****************************/
enum {
	fullSendPacket=0,
	normalInnerModulePacket
};
typedef TypedPacket<char, PacketBase> TypedInnerModulePacket;
typedef EmptyPacket FullSendPacket;


class TypedInnerModulePacketFactory : public TypedPacketFactory<char,PacketBase> {
public:
	TypedInnerModulePacketFactory( MulticrewModule *mod ) {
		this->mod = mod;
	}

	SmartPtr<PacketBase> createPacket( char key, SharedBuffer &buffer ) {
		switch( key ) {
		case normalInnerModulePacket: return mod->createInnerModulePacket( buffer );
		case fullSendPacket: return new FullSendPacket();
		default:
			dout << "Invalid module packet type " << (int)key << std::endl;
			break;
		}

		return 0;
	}

private:
	MulticrewModule *mod;
};
    

/*******************************************************************/
struct MulticrewModule::Data {
	Data( MulticrewModule *mod ) 
		: con( __FILE__, __LINE__ ),
		innerModuleFactory(mod) {
	}

	SmartPtr<MulticrewCore> core;
	std::string moduleName;
	SmartPtr<Connection> con;
	SmartPtr<FileConfig> config;
	TypedInnerModulePacketFactory innerModuleFactory;

	// packet send data
	SmartPtr<ModulePacket> packet;
	unsigned fragmentStart;
	int packetSize;
	unsigned maxPacketSize;
	bool nextIsSafeTransmission;
	unsigned minSendWait;
	Connection::Priority nextPriority;
	CRITICAL_SECTION sendCritSect;
};


MulticrewModule::MulticrewModule( std::string moduleName, 
								  unsigned minSendWait ) {
	d = new Data( this );
	InitializeCriticalSection( &d->sendCritSect );
	d->moduleName = moduleName;
	d->minSendWait = minSendWait;
	d->core = MulticrewCore::multicrewCore();
	
	// packet setup
	d->packet = new ModulePacket();
	d->nextPriority = Connection::lowPriority;
	d->nextIsSafeTransmission = false;

	MulticrewCore::multicrewCore()->registerModule( this );
}


MulticrewModule::~MulticrewModule() {
	dout << "~MulticrewModule()" << std::endl;
	disconnect();	
	MulticrewCore::multicrewCore()->unregisterModule( this );

	DeleteCriticalSection( &d->sendCritSect );
	delete d;
}


std::string MulticrewModule::moduleName() {
	return d->moduleName;
}


void MulticrewModule::receive( SmartPtr<ModulePacket> packet ) {
	// handle all inner packets
	for( ModulePacket::iterator it = packet->begin();
		 it!=packet->end();
		 it++ ) {
		SmartPtr<TypedInnerModulePacket> tp = (TypedInnerModulePacket*)&*(*it);

		// which packet type?
		switch( tp->key() ) {
		case fullSendPacket:
			sendFullState();
			break;
		case normalInnerModulePacket:
			handlePacket( tp->wrappee() );
			break;
		default:
			dout << "Received invalid inner module packet of type " << (int)tp->key() << std::endl;
			break;
		}
	}
}

void MulticrewModule::lock() {
	EnterCriticalSection( &d->sendCritSect );
}

void MulticrewModule::send( SmartPtr<PacketBase> packet, bool safe, 
							Connection::Priority prio ) {
	if( !d->con.isNull() ) {
		EnterCriticalSection( &d->sendCritSect );

		// append packet
		d->packet->append( new TypedInnerModulePacket( 
							   normalInnerModulePacket,
							   packet ) );
		
		// update packet mode
		d->nextIsSafeTransmission = d->nextIsSafeTransmission || safe;
		if( (d->nextPriority==Connection::lowPriority && prio==Connection::mediumPriority) ||
			d->nextPriority!=Connection::highPriority && prio==Connection::highPriority)
			d->nextPriority = prio;

		LeaveCriticalSection( &d->sendCritSect );
	}
}

bool MulticrewModule::sendAsync( SmartPtr<PacketBase> packet, bool safe, 
								 Connection::Priority prio, int channel ) {
	if( !d->con.isNull() ) {
		SmartPtr<ModulePacket> modPacket = new ModulePacket();
		modPacket->append( new TypedInnerModulePacket(
							   normalInnerModulePacket,
							   packet ) );
		return d->con->send( 
			modPacket, 
			safe, 
			prio, 
			this,
			true, // async
			channel );
	}

	return false;
}

void MulticrewModule::requestFullState() {
	d->packet->append( new TypedInnerModulePacket( 
						   fullSendPacket,
						   new FullSendPacket() ) );	
}

void MulticrewModule::unlock() {
	LeaveCriticalSection( &d->sendCritSect );
}

void MulticrewModule::sendCompleted() {
	//dout << moduleName() << " send completed" << std::endl;
}


void MulticrewModule::sendFailed() {
	dout << moduleName() << " send failed" << std::endl;
}


void MulticrewModule::connect( SmartPtr<Connection> con ) {
	dout << moduleName() << " connecting" << std::endl;
	d->con = con;
	d->con->addModule( this );
	
    // start thread
	if( d->minSendWait>0 ) {
		startThread( 0 );
	}

	// request full state from other side
	requestFullState();
}


void MulticrewModule::disconnect() {
	if( !d->con.isNull() ) {
		dout << moduleName() << " disconnecting" << std::endl;
		stopThread();
		
		// disconnect from connection
		d->con->removeModule( this );
		d->con = 0;
	}
}


void MulticrewModule::sendProc() {
	// do nothing by default
}


SmartPtr<PacketBase> MulticrewModule::createPacket( SharedBuffer &buffer ) {
	return new TypedInnerModulePacket( buffer, &d->innerModuleFactory );
}


unsigned MulticrewModule::threadProc( void *param ) {
	dout << moduleName() << " thread started" << std::endl;

	while( true ) {
		// prepare packet
		sendProc();

		// copy current packet
		EnterCriticalSection( &d->sendCritSect );
		SmartPtr<ModulePacket> packet = d->packet;
		bool nextIsSafeTransmission = d->nextIsSafeTransmission;
		Connection::Priority nextPriority = d->nextPriority;
		
		// setup for next packet
		d->packet = new ModulePacket;
		d->nextPriority = Connection::lowPriority;
		d->nextIsSafeTransmission = false;
		LeaveCriticalSection( &d->sendCritSect );

		// send packet
		if( packet->size()>0 ) {
			d->con->send( 
				packet, 
				nextIsSafeTransmission, 
				nextPriority, 
				this );
		}
			
		// wait an amount of milliseconds or exit thread
		if( shouldExit( d->minSendWait ) )
			break;
	}
	
    // exit the thread
	dout << moduleName() << " thread exits" << std::endl;
	return 0;
}


SmartPtr<FileConfig> MulticrewModule::config() {
	lock();
	if( d->config.isNull() )
		d->config = new FileConfig( "multicrew/" + moduleName() + ".ini" );
	SmartPtr<FileConfig> ret = d->config;   
	unlock();
	return ret;
}
