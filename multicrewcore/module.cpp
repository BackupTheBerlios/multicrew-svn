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
		: con( __FILE__, __LINE__ ),
		  threadProcAdapter( mod, MulticrewModule::threadProc ) {
	}

	SmartPtr<MulticrewCore> core;
	std::string moduleName;
	bool hostMode;
	bool registered;
	SmartPtr<Connection> con;

	unsigned minSendWait;
	volatile bool lastPacketSent;
	CallbackAdapter1<DWORD, MulticrewModule, LPVOID> threadProcAdapter;
	HANDLE thread;
	HANDLE exitEvent;
	HANDLE exitedEvent;

	// packet send data
	SmartPtr<ModulePacket> packet;
	unsigned fragmentStart;
	int packetSize;
	unsigned maxPacketSize;
	bool nextIsSafeTransmission;
	Connection::Priority nextPriority;
	CRITICAL_SECTION sendCritSect;
};

MulticrewModule::MulticrewModule( std::string moduleName, bool hostMode, 
								  unsigned minSendWait ) {
	d = new Data( this );
	d->moduleName = moduleName;
	d->hostMode = hostMode;
	d->minSendWait = minSendWait;
	d->thread = 0;
	d->core = MulticrewCore::multicrewCore();
	d->registered = d->core->registerModule( this );

	// packet setup
	d->packet = new ModulePacket();
	d->lastPacketSent = true;
	d->nextPriority = Connection::LowPriority;
	d->nextIsSafeTransmission = false;

	InitializeCriticalSection( &d->sendCritSect );
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
		if( (d->nextPriority==Connection::LowPriority && prio==Connection::MediumPriority) ||
			d->nextPriority!=Connection::HighPriority && prio==Connection::HighPriority)
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
		d->exitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
		d->exitedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
		d->thread = CreateThread(
			NULL,
			0,
			d->threadProcAdapter.callback(),
			0,
			0,
			NULL );
		if( d->thread==NULL ) {
			CloseHandle( d->exitEvent );
			d->exitEvent = 0;
			dlog << "Cannot create FSUIPC thread" << std::endl;
		}
	}
}


void MulticrewModule::disconnect() {
	dout << moduleName() << " disconnecting" << std::endl;
	if( !d->con.isNull() ) {

		// stop thread
		if( d->thread!=0 ) { 
			SetEvent( d->exitEvent );
			WaitForSingleObject( d->exitedEvent, INFINITE );
			dout << moduleName() << " has exited" << std::endl;
			CloseHandle( d->thread );
			CloseHandle( d->exitEvent );
			CloseHandle( d->exitedEvent );
			d->thread = 0;
		}
		
		// disconnect from connection
		d->con->removeModule( this );
		d->con = 0;
	}

	dout << moduleName() << " disconnected" << std::endl;
}


void MulticrewModule::sendProc() {
	// do nothing by default
}


DWORD MulticrewModule::threadProc( LPVOID param ) {
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
			d->nextPriority = Connection::LowPriority;
			d->nextIsSafeTransmission = false;
		}
			
		// wait an amount of milliseconds or exit thread
		if( WaitForSingleObject( d->exitEvent, d->minSendWait )==WAIT_OBJECT_0 )
			break;
	}

	dout << moduleName() << " thread exits" << std::endl;
	SetEvent( d->exitedEvent );
	
    // exit the thread
	return 0;
}
