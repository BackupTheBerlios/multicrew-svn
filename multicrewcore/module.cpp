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

#pragma pack(push,1)
struct DataFragment {
	DWORD size;
	char data[0];
};
#pragma pack(pop,1)


struct MulticrewModule::Data {
	Data( MulticrewModule *mod ) 
		: con( __FILE__, __LINE__ ),
		  threadProcAdapter( mod, MulticrewModule::threadProc ) {
	}

	std::string moduleName;
	bool hostMode;
	bool registered;
	SmartPtr<Connection> con;

	unsigned minSendWait;
	volatile bool sent;
	CallbackAdapter1<DWORD, MulticrewModule, LPVOID> threadProcAdapter;
	HANDLE thread;
	HANDLE exitEvent;
	HANDLE exitedEvent;

	ModulePacket *packet;
	unsigned maxPacketSize;
	bool safe;
	Connection::Priority priority;
};

MulticrewModule::MulticrewModule( std::string moduleName, bool hostMode, 
								  unsigned minSendWait ) {
	d = new Data( this );
	d->moduleName = moduleName;
	d->hostMode = hostMode;
	d->minSendWait = minSendWait;
	d->thread = 0;
	d->sent = true;
	d->maxPacketSize = 8192;
	d->packet = (ModulePacket*)malloc(d->maxPacketSize);
	strncpy( d->packet->module, this->moduleName().c_str(), 32 );
	d->packet->id = modulePacket;
	d->registered = MulticrewCore::multicrewCore()->registerModule( this );
}


MulticrewModule::~MulticrewModule() {
	dout << "~MulticrewModule()" << std::endl;
	disconnect();

	// unregister from core
	if( d->registered ) MulticrewCore::multicrewCore()->unregisterModule( this );

	delete d->packet;
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


void MulticrewModule::receive( ModulePacket *packet ) {
	// iterate over all fragments
	unsigned pos = sizeof(ModulePacket);
	while( pos<packet->size ) {
		DataFragment *fragment = (DataFragment*)(((char*)packet) + pos);
		receive( fragment->data, fragment->size );
		pos += fragment->size+sizeof(DataFragment);
	}
}


void MulticrewModule::send( void *data, DWORD size,	bool safe, Connection::Priority prio ) {
	if( !d->con.isNull() ) {
		// create packet buffer of sufficient size
		if( d->maxPacketSize<d->packet->size+sizeof(DataFragment)+size ) {
			while( d->maxPacketSize<d->packet->size+sizeof(DataFragment)+size )
				d->maxPacketSize *= 2;
			
			d->packet = (ModulePacket*)realloc( d->packet, d->maxPacketSize );
		}
		
		// append data to packet
		DataFragment *dest = (DataFragment*)(((char*)d->packet)+d->packet->size);
		dest->size = size;
		memcpy( dest->data, data, size );
		d->packet->size += sizeof(DataFragment)+size;
		
		// update packet mode
		d->safe = d->safe || safe;
		if( (d->priority==Connection::LowPriority && prio==Connection::MediumPriority) ||
			d->priority!=Connection::HighPriority && prio==Connection::HighPriority)
			d->priority = prio;
	}
}


void MulticrewModule::sendCompleted( Packet *packet ) {
	//dout << moduleName() << " send completed" << std::endl;
	d->sent = true;
}


void MulticrewModule::sendFailed( Packet *packet ) {
	dout << moduleName() << " send failed" << std::endl;
	d->sent = true;
}


std::string MulticrewModule::id() {
	return d->moduleName;
}


void MulticrewModule::connect( SmartPtr<Connection> con ) {
	dout << moduleName() << " connecting" << std::endl;
	d->con = con;
	d->con->addReceiver( this );
	d->sent = true;

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
		d->con->removeReceiver( this );
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
		if( d->sent ) {
			// default values
			d->packet->size = sizeof(ModulePacket);
			d->priority = Connection::LowPriority;
			d->safe = false;

			// prepare packet
			sendProc();

			// send packet
			if( d->packet->size>sizeof(ModulePacket) ) {
				d->sent = false;
				if( !d->con->send( d->packet, d->safe, d->priority, this ) )
					d->sent = true;
			} else {
				d->sent = true;
			}
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
