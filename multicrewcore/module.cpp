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



/*******************************************************************/


struct MulticrewModule::Data {
	Data( MulticrewModule *mod ) 
		: con( __FILE__, __LINE__ ) {
	}

	SmartPtr<MulticrewCore> core;
	std::string moduleName;
	bool hostMode;
	bool registered;
	SmartPtr<Connection> con;
	SmartPtr<FileConfig> config;

	// packet send data
	SmartPtr<ModulePacket> packet;
	unsigned fragmentStart;
	int packetSize;
	unsigned maxPacketSize;
	bool nextIsSafeTransmission;
	volatile bool lastPacketSent;
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
	d->lastPacketSent = true;
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
	ModulePacket::iterator it = packet->begin();
	while( it!=packet->end() ) {
		handlePacket( *it );
		it++;
	}
}

void MulticrewModule::lock() {
	EnterCriticalSection( &d->sendCritSect );
}

void MulticrewModule::send( SmartPtr<Packet> packet, bool safe, Connection::Priority prio ) {
	if( !d->con.isNull() ) {
		// append packet
		d->packet->append( packet );
		
		// update packet mode
		d->nextIsSafeTransmission = d->nextIsSafeTransmission || safe;
		if( (d->nextPriority==Connection::lowPriority && prio==Connection::mediumPriority) ||
			d->nextPriority!=Connection::highPriority && prio==Connection::highPriority)
			d->nextPriority = prio;
	}
}

void MulticrewModule::unlock() {
	LeaveCriticalSection( &d->sendCritSect );
}

void MulticrewModule::sendCompleted() {
	//dout << moduleName() << " send completed" << std::endl;
	d->lastPacketSent = true;
}


void MulticrewModule::sendFailed() {
	dout << moduleName() << " send failed" << std::endl;
	d->lastPacketSent = true;
}


void MulticrewModule::connect( SmartPtr<Connection> con ) {
	dout << moduleName() << " connecting" << std::endl;
	d->con = con;
	d->con->addModule( this );
	d->lastPacketSent = true;
	
    // start thread
	if( d->minSendWait>0 ) {
		startThread( 0 );
	}
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


unsigned MulticrewModule::threadProc( void *param ) {
	dout << moduleName() << " thread started" << std::endl;

	while( true ) {
		// call send proc if previous packet has arrived
		if( d->lastPacketSent ) {
			// prepare packet
			sendProc();

			// send packet
			if( d->packet->size()>0 ) {
				d->lastPacketSent = false;
				if( !d->con->send( 
						d->packet, 
						d->nextIsSafeTransmission, d->nextPriority, this ) )
					d->lastPacketSent = true;
			} else {
				d->lastPacketSent = true;
			}

			// default values
			d->packet = new ModulePacket;
			d->nextPriority = Connection::lowPriority;
			d->nextIsSafeTransmission = false;
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
