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

#include "common.h"

#include <windows.h>

#include "streams.h"
#include "log.h"
#include "network.h"
#include "callback.h"


/*******************************************************************/


struct Connection::Data {
	Data( Connection *con ) {}

	CRITICAL_SECTION critSect;
	std::string error;
	State state;
};


Connection::Connection() {
	d = new Data( this );
	d->state = idleState;
	d->error = "";
	InitializeCriticalSection( &d->critSect );
}


Connection::~Connection() {
	stopThread();
	DeleteCriticalSection( &d->critSect );
	delete d;
}


unsigned Connection::threadProc( void *param ) {
	while( !shouldExit(10) ) {
		processImpl();
	}
	return 0;
}


void Connection::setState( State state, std::string error ) {
	d->error = error;
	if( state!=d->state ) { 
		d->state = state;
		switch( d->state ) {
		case idleState:
			stopThread();
			break;
			
		case connectingState:
			startThread( 0 );
			break;
			
		case connectedState:
			startThread( 0 );
			break;

		case disconnectedState: {
			ref();
			triggerAsyncCallback();
		} break;
		}
	}
}

void Connection::processPacket( void *data, unsigned length ) {
	SharedBuffer packetBuf( (char*)data, length );
	MulticrewCore::multicrewCore()->receive( packetBuf );
}


void Connection::asyncCallback() {
	disconnected.emit( d->error );
	deref();
}


std::string Connection::error() {
	return d->error;
}


Connection::State Connection::state() {
	return d->state;
}


bool Connection::send( SmartPtr<PacketBase> packet, bool safe, 
					   Priority prio, unsigned channel ) {
	// connected?
	if( state()!=connectedState ) return false;

	// prepare packet
	unsigned size = packet->compiledSize()+1;
	char *buffer = (char*)malloc( size );
	packet->compile( buffer+1 );
	buffer[0]=127;

	// set priority
	PacketPriority pprio = MEDIUM_PRIORITY;
	switch( prio ) {
	case lowPriority: pprio=LOW_PRIORITY; break;
	case mediumPriority: pprio=MEDIUM_PRIORITY; break;
	case highPriority: pprio=HIGH_PRIORITY; break;
	default: break;
	}

	// set reliability mode
	PacketReliability reliability;
	if( safe )
		reliability = RELIABLE_ORDERED;
	else
		reliability = RELIABLE_SEQUENCED;
				
	// send
	bool ok = sendImpl( (char*)buffer, 
						size, 
						pprio, 
						reliability, 
						channel );
	
	// free packet buffer
	free( buffer );

    // error?
	if( !ok ) {
		dout << "Failed to send packet." << std::endl;
		return false;
	}

	return true;
}
