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
#include "../multicrewgauge/gauges.h"

#include "streams.h"
#include "position.h"
#include "log.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 100


struct PositionStruct {
	DWORD lat[2];
	DWORD lon[2];
	DWORD alt[2];
	DWORD dir[3];
	FLOAT64 accel[6];
	FLOAT64 vel[6];
};

typedef StructPacket<PositionStruct> PositionPacket;


PositionModule::PositionModule( bool hostMode ) 
	: MulticrewModule( "FSUIPCPos", hostMode, hostMode?WAITTIME:-1 ) {
	MulticrewCore::multicrewCore()->registerModule( this );
}


PositionModule::~PositionModule() {
	MulticrewCore::multicrewCore()->unregisterModule( this );
}


SmartPtr<Packet> PositionModule::createPacket( SharedBuffer &buffer ) {
	return new PositionPacket( buffer );
}


/***********************************************************************/


struct PositionHostModule::Data {
	Data( PositionHostModule *mod ) {
	}
};


PositionHostModule::PositionHostModule() 
	: PositionModule( true ) {
	d = new Data( this );
	
	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) )
		dlog << "Cannot connect to FSUIPC" << std::endl;
}


PositionHostModule::~PositionHostModule() {
	// disconnect from FSUIPC
	FSUIPC_Close();

	delete d;
}


void PositionHostModule::sendProc() {
    // read variables from the FS
	PositionStruct data;
	DWORD res;
	bool ok = true;
	ok = ok && FSUIPC_Read( 0x560, 36, data.lat, &res );
	ok = ok && FSUIPC_Read( 0x3060, 8*12, data.accel, &res );
	ok = ok && FSUIPC_Process( &res );
	if( !ok )
		dout << "FSUIPC read error" << std::endl;
	else
		// and send packet
		send( new PositionPacket(data), false, Connection::MediumPriority );
}


/***********************************************************************/


struct PositionClientModule::Data {
	Data( PositionClientModule *mod ) {
	}
};

PositionClientModule::PositionClientModule() 
	: PositionModule( false ) {
	d = new Data( this );

	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) ) {
		dlog << "Cannot connect to FSUIPC" << std::endl;
	}
}


PositionClientModule::~PositionClientModule() {
	// disconnect from FSUIPC
	FSUIPC_Close();
	
	delete d;
}


void PositionClientModule::handlePacket( SmartPtr<Packet> packet ) {
	SmartPtr<PositionPacket> pp = (PositionPacket*)&*packet;
	//dout << "position packet received" << std::endl;
	
	// write variables from the FS
	DWORD res;
	bool ok = true;
	ok = ok && FSUIPC_Write( 0x560, 36, pp->data().lat, &res );
	ok = ok && FSUIPC_Write( 0x3060, 8*12, pp->data().accel, &res );
	ok = ok && FSUIPC_Process( &res );
	if( !ok ) {
		dout << "FSUIPC write error" << std::endl;
	}
}
