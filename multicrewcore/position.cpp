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
#include "fsuipc/FSUIPC_User.h"

#include "position.h"
#include "error.h"
#include "log.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 100


PositionModule::PositionModule( bool hostMode ) 
	: MulticrewModule( "FSUIPCPos", hostMode ) {
	MulticrewCore::multicrewCore()->registerModule( this );
}


PositionModule::~PositionModule() {
	MulticrewCore::multicrewCore()->unregisterModule( this );
}

/***********************************************************************/

struct PositionHostModule::Data {
	Data( PositionHostModule *mod ) 
		: threadProcAdapter( mod, PositionHostModule::threadProc ) {
	}

	bool connected;
	CallbackAdapter1<DWORD, PositionHostModule, LPVOID> threadProcAdapter;
	HANDLE thread;
	bool exitFlag;
};


PositionHostModule::PositionHostModule() 
	: PositionModule( true ) {
	d = new Data( this );
	d->connected = false;
	d->thread = 0;
	d->exitFlag = false;
}


PositionHostModule::~PositionHostModule() {
	// stop thread
	d->exitFlag = true;
	SleepEx( WAITTIME*3, FALSE );
    // todo: wait for thread to exit

	// disconnect from FSUIPC
	if( d->connected ) FSUIPC_Close();
	delete d;
}


bool PositionHostModule::start() {
	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) ) {
		log << "Cannot connect to FSUIPC" << std::endl;
		return false;
	}
	d->connected = true;

	// start thread
	d->thread = CreateThread(
		NULL,
		0,
		d->threadProcAdapter.callback(),
		0,
		0,
		NULL );
	if( d->thread==NULL ) {
		log << "Cannot create FSUIPC thread" << std::endl;
		return false;
	}

	return true;
}


DWORD PositionHostModule::threadProc( LPVOID param ) {
	// run as long the flag is not set
	while( d->exitFlag==false ) {
		PositionPacket packet( moduleName() );

		// read variables from the FS
		DWORD res;
		bool ok = true;
		ok = ok && FSUIPC_Read( 0x560, 36, packet.lat, &res );
		ok = ok && FSUIPC_Read( 0x3060, 8*12, packet.accel, &res );
		ok = ok && FSUIPC_Process( &res );
		if( !ok ) {
			dout << "FSUIPC read error" << std::endl;
		} else {
			// send through the network
			//dout << "send position packet" << std::endl;
			send( &packet, false, false );
		}

		// wait an amount of milliseconds
		if( !d->exitFlag ) SleepEx( WAITTIME, FALSE );
	}
	
    // exit the thread
	ExitThread( 0 );
}

/***********************************************************************/

struct PositionClientModule::Data {
	Data( PositionClientModule *mod ) {
	}

	bool connected;
};

PositionClientModule::PositionClientModule() 
	: PositionModule( false ) {
	d = new Data( this );
	d->connected = false;
}


PositionClientModule::~PositionClientModule() {
	// disconnect from FSUIPC
	if( d->connected ) FSUIPC_Close();
	delete d;
}


bool PositionClientModule::start() {
	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) ) {
		log << "Cannot connect to FSUIPC" << std::endl;
		return false;
	}
	d->connected = true;

	return true;
}

 void PositionClientModule::receive( ModulePacket *packet ) {
	 PositionPacket *pp = (PositionPacket*)packet;
	 //dout << "position packet received" << std::endl;
	 
	 // write variables from the FS
	 DWORD res;
	 bool ok = true;
	 ok = ok && FSUIPC_Write( 0x560, 36, pp->lat, &res );
	 ok = ok && FSUIPC_Write( 0x3060, 8*12, pp->accel, &res );
	 ok = ok && FSUIPC_Process( &res );
	 if( !ok ) {
		 dout << "FSUIPC write error" << std::endl;
	 }
}
