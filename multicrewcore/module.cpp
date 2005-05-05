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
	bool hostMode;
	bool registered;
	SmartPtr<Connection> con;
	SmartPtr<FileConfig> config;
	TypedInnerModulePacketFactory innerModuleFactory;

	// packet send data
	SmartPtr<ModulePacket> packet;
	unsigned fragmentStart;
	int packetSize;
	unsigned maxPacketSize;
	bool nextIsSafeTransmission;
	volatile int packetsSending;
	unsigned minSendWait;
	Connection::Priority nextPriority;
	CRITICAL_SECTION sendCritSect;
};


MulticrewModule::MulticrewModule( std::string moduleName, bool hostMode, 
								  unsigned minSendWait ) {
	d = new Data( this );
	InitializeCriticalSection( &d->sendCritSect );
	d->moduleName = moduleName;
	d->hostMode = hostMode;
	d->minSendWait = minSendWait;
	d->core = MulticrewCore::multicrewCore();
	d->registered = d->core->registerModule( this );

	// packet setup
	d->packet = new ModulePacket();
	d->packetsSending = 0;
	d->nextPriority = Connection::lowPriority;
	d->nextIsSafeTransmission = false;
}


MulticrewModule::~MulticrewModule() {
	dout << "~MulticrewModule()" << std::endl;
	disconnect();

	// unregister from core
	if( d->registered ) MulticrewCore::multicrewCore()->unregisterModule( this );

	DeleteCriticalSection( &d->sendCritSect );
	delete d;
}


bool::MulticrewModule::registered() {
	return d->registered;
}


std::string MulticrewModule::moduleName() {
	return d->moduleName;
}


bool MulticrewModule::isHostMode() {
	return d->hostMode;
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
		// append packet
		d->packet->append( new TypedInnerModulePacket( 
							   normalInnerModulePacket,
							   packet ) );
		
		// update packet mode
		d->nextIsSafeTransmission = d->nextIsSafeTransmission || safe;
		if( (d->nextPriority==Connection::lowPriority && prio==Connection::mediumPriority) ||
			d->nextPriority!=Connection::highPriority && prio==Connection::highPriority)
			d->nextPriority = prio;
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
	EnterCriticalSection( &d->sendCritSect );
	d->packetsSending--;
	LeaveCriticalSection( &d->sendCritSect );
}


void MulticrewModule::sendFailed() {
	dout << moduleName() << " send failed" << std::endl;
	EnterCriticalSection( &d->sendCritSect );
	d->packetsSending--;
	LeaveCriticalSection( &d->sendCritSect );
}


void MulticrewModule::connect( SmartPtr<Connection> con ) {
	dout << moduleName() << " connecting" << std::endl;
	d->con = con;
	d->con->addModule( this );
	d->packetsSending = 0;
	
    // start thread
	if( d->minSendWait>0 ) {
		startThread( 0 );
	}

	// request full state from other side
	requestFullState();
}


void MulticrewModule::disconnect() {
	dout << moduleName() << " disconnecting" << std::endl;
	if( !d->con.isNull() ) {
		stopThread();
		
		// disconnect from connection
		d->con->removeModule( this );
		d->con = 0;
	}

	dout << moduleName() << " disconnected" << std::endl;
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
		// call send proc if previous packet has arrived
		if( d->packetsSending==0 ) {
			// prepare packet
			sendProc();

			// send packet
			EnterCriticalSection( &d->sendCritSect );
			if( d->packet->size()>0 ) {
				d->packetsSending++;
				LeaveCriticalSection( &d->sendCritSect );
				if( !d->con->send( 
						d->packet, 
						d->nextIsSafeTransmission, 
						d->nextPriority, 
						this ) ) {
					EnterCriticalSection( &d->sendCritSect );
					d->packetsSending--;					
					LeaveCriticalSection( &d->sendCritSect );
				}
				EnterCriticalSection( &d->sendCritSect );
			} 			

			// default values
			d->packet = new ModulePacket;
			d->nextPriority = Connection::lowPriority;
			d->nextIsSafeTransmission = false;
			LeaveCriticalSection( &d->sendCritSect );
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
