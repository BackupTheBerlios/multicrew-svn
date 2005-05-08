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

#include "../multicrewgauge/gauges.h"
#include "streams.h"
#include "position.h"
#include "log.h"
#include "fsuipc.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 100

/******************* packets **************************/
struct PositionStruct {
	double timestamp; // time since last packet
	FLOAT64 pos[3];
	DWORD dir[3];
//	FLOAT64 accel[6];
	FLOAT64 vel[6];
};

typedef StructPacket<PositionStruct> PositionPacket;


/********************************************************/
struct PositionModule::Data {
	Data( PositionModule *mod ) {
		fsuipc = Fsuipc::fsuipc();
	}

	SmartPtr<Fsuipc> fsuipc;
	SmartPtr<MulticrewCore> core;

	/* host mode */
	PositionStruct data;

    /* client mode */
	double timediff; // mean value of time difference to host
	double meanLatency; // mean value of latency derivation
};


PositionModule::PositionModule() 
	: MulticrewModule( "FSUIPCPos", WAITTIME ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();

	/* client mode */
	d->timediff = 0;
	d->meanLatency = 0;
}


PositionModule::~PositionModule() {
	delete d;
}


SmartPtr<PacketBase> PositionModule::createInnerModulePacket( SharedBuffer &buffer ) {
	return new PositionPacket( buffer );
}


void PositionModule::sendFullState() {
}


void PositionModule::sendProc() {
	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		// calculate time difference to previous packet
		d->data.timestamp = MulticrewCore::multicrewCore()->time();

		// read variables from the FS	
		d->fsuipc->begin();	
		bool ok = d->fsuipc->read( 0x560, 9*sizeof(DWORD), d->data.pos );
		//ok = ok && d->fsuipc->read( 0x3060, 6*8, data.accel );
		ok = ok && d->fsuipc->read( 0x3090, 6*sizeof(FLOAT64), d->data.vel );
		ok = ok && d->fsuipc->end();
		if( !ok )
			dout << "FSUIPC position read error" << std::endl;
		else
			// and send packet
			send( new PositionPacket(d->data), false, Connection::mediumPriority );
	} break;
	case MulticrewCore::ClientMode: break;
	}
}


void PositionModule::handlePacket( SmartPtr<PacketBase> packet ) {
	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<PositionPacket> pp = (PositionPacket*)&*packet;
		//dout << "position packet received" << std::endl;
		
		// calculate time difference to host and latency
		double now = MulticrewCore::multicrewCore()->time() - d->timediff;
		double latency = now-pp->data().timestamp;
		d->timediff += latency/10.0;
		d->meanLatency = (d->meanLatency*9.0 + ((latency<0)?-latency:latency))/10.0;
		//dout << "timediff=" << d->timediff << " meanLat=" << d->meanLatency << " latency=" << latency << std::endl;
		
		
		// interpolate
        /*	FLOAT64 pos[3];
	    pos[0] = pp->data().pos[0] + latency*65536.0*65536.0*pp->data().vel[1]/0.3048;
	    pos[1] = pp->data().pos[1] + latency*65536.0*65536.0*65536.0*65536.0
	    pp->data().vel[0]/0.3048;*/
		
		// write variables from the FS
		if( latency<0 ) latency=-latency;
		if( latency<d->meanLatency ) {
			d->fsuipc->begin();
			bool ok = d->fsuipc->write( 0x560, 9*sizeof(DWORD), pp->data().pos );
			//ok = ok && d->fsuipc->write( 0x3060, 6*8, pp->data().accel );
			ok = ok && d->fsuipc->write( 0x3090, 6*sizeof(FLOAT64), pp->data().vel );
			ok = ok && d->fsuipc->end();
			if( !ok ) {
				dout << "FSUIPC position write error" << std::endl;
			}
		}
	} break;
	}
}
