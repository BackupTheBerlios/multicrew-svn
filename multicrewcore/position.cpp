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

#include <math.h>
#include <windows.h>

#include "../multicrewgauge/gauges.h"
#include "streams.h"
#include "position.h"
#include "log.h"
#include "fsuipc.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 150


/******************* packets **************************/
#pragma pack(push,1)
struct PositionStruct {
	double timestamp; // time since last packet
	__int64 pos[3];
	signed __int32 dir[3];
};
#pragma pack(pop)


typedef StructPacket<PositionStruct> PositionPacket;


/********************************************************/
struct PositionModule::Data {
	Data( PositionModule *mod ) 
		: frameSlot( 0, mod, PositionModule::frameCallback ) {
	}

	SmartPtr<Fsuipc> fsuipc;
	SmartPtr<MulticrewCore> core;
	CRITICAL_SECTION cs;
	Slot<PositionModule> frameSlot;

    /* client mode */
	double timediff; // mean value of time difference to host
	double meanLatency; // mean value of latency derivation
	double lastReferenceTime;

	bool extrapolate;
	PositionStruct reference;
	PositionStruct extrapolated;
	__int64 posVel[3];	
	double dirVel[3];	
};


PositionModule::PositionModule() 
	: MulticrewModule( "FSUIPCPos" ), NetworkChannel( "FSUIPCPos" ) {
	d = new Data( this );
	InitializeCriticalSection( &d->cs );
	d->core = MulticrewCore::multicrewCore();
	d->extrapolate = false;
	d->frameSlot.connect( &d->core->frameSignal );
}


PositionModule::~PositionModule() {
	DeleteCriticalSection( &d->cs );
	delete d;
}


void PositionModule::start() {
	if( d->fsuipc.isNull() ) d->fsuipc = Fsuipc::fsuipc();
}


void PositionModule::stop() {
}


SmartPtr<PacketBase> PositionModule::createPacket( SharedBuffer &buffer ) {
	return new PositionPacket( buffer );
}


void PositionModule::sendFullState() {
}


double x_to_lat( __int64 x ) {
	return x*90.0/(10001750.0 * 65536.0 * 65536.0);
}


double y_to_lon( __int64 y ) {
	return y*360.0/(65536.0 * 65536.0 * 65536.0 * 65536.0);
}


double z_to_ft( __int64 z ) {
	return z*1.0/(65536.0 * 65536.0)/0.304800;
}


double z_to_m( __int64 z ) {
	return z*1.0/(65536.0 * 65536.0);
}


double dlat_to_m( double dlat ) {
	return dlat*40076592/360.0;
}


double dir_to_deg( __int64 dir ) {
	return dir/65536.0/65536.0*360.0;
}


__int64 deg_to_dir( double deg ) {
	while( deg>180.0 ) deg -= 360.0;
	while( deg<=-180.0 ) deg += 360.0;
	return (__int64)(deg/360.0*65536.0*65536.0);
}


double dlon_to_m( double dlon, double lat ) {
	return dlon/360.0*40076592.0*cos( lat );
}


double sqr( double x ) {
	return x*x;
}


std::string to_string( __int64 (&s)[3] ) {
	return to_string( x_to_lat(s[0]) )
		+  ":" 
		+ to_string( y_to_lon(s[1]) )
		+ ":" 
		+ to_string( z_to_ft(s[2]) );
}


std::string to_string( double (&s)[3] ) {
	return to_string( s[0] )
		+  ":" 
		+ to_string( s[1] )
		+ ":" 
		+ to_string( s[2] );
}


std::string to_string( signed __int32 (&s)[3] ) {
	return to_string( dir_to_deg(s[0]) )
		+  ":" 
		+ to_string( dir_to_deg(s[1]) )
		+ ":" 
		+ to_string( dir_to_deg(s[2]) );
}



void PositionModule::frameCallback() {
	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: 
		d->extrapolate = false;
		break;
	case MulticrewCore::HostMode: 
	if( !d->fsuipc.isNull() ) {
		d->extrapolate = false;
		double timeSinceLastPacket = 
			MulticrewCore::multicrewCore()->time()-d->lastReferenceTime;
		if( timeSinceLastPacket>WAITTIME/1000.0 ) {
			// read variables from the FS
			PositionStruct pos;
			d->fsuipc->begin();	
			bool ok1 = d->fsuipc->read( 0x560, 9*sizeof(DWORD), pos.pos );
			bool ok2 = d->fsuipc->end();
			if( !ok1 || !ok2 )
				dout << "FSUIPC position read error" << std::endl;
			else {
				// and send packet
				pos.timestamp = MulticrewCore::multicrewCore()->time();
				send( new PositionPacket(pos), false, highPriority );
				d->lastReferenceTime = MulticrewCore::multicrewCore()->time();
			}
		}
	} break;
	case MulticrewCore::ClientMode:
		if( !d->fsuipc.isNull() ) {
			if( d->extrapolate ) {
				// extrapolate
				EnterCriticalSection( &d->cs );
				double now = MulticrewCore::multicrewCore()->time();
				double ago = now-d->lastReferenceTime;
				for( int x=0; x<3; x++ ) {
					d->extrapolated.pos[x] = d->reference.pos[x] + (__int64)(ago*d->posVel[x]);
					d->extrapolated.dir[x] = deg_to_dir(dir_to_deg(d->reference.dir[x]) + ago*d->dirVel[x]);
				}
				//dout << to_string(d->reference.pos) 
				//<< " --> " << to_string(d->extrapolated.pos) << std::endl;
				PositionStruct value = d->extrapolated;
				LeaveCriticalSection( &d->cs );	
				
				// write extrapolated position to FS9
				d->fsuipc->begin();
				bool ok1 = d->fsuipc->write( 0x560, 9*sizeof(DWORD), value.pos );
				bool ok2 = d->fsuipc->end();
				if( !ok1 || !ok2 ) {
					dout << "FSUIPC position write error" << std::endl;
				}						
			}
		}
		break;
	}
}


void PositionModule::receive( SmartPtr<PacketBase> packet ) {
	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: {
		SmartPtr<PositionPacket> pp = (PositionPacket*)&*packet;
		PositionStruct &newPos = pp->data();
		
		// calculate time difference to host and latency
		double now = MulticrewCore::multicrewCore()->time();
		double latency;
		if( !d->extrapolate ) {
			latency = 0;
			d->timediff = MulticrewCore::multicrewCore()->time() - newPos.timestamp;
			d->meanLatency = 0;
		} else {
			double correctedNow = MulticrewCore::multicrewCore()->time() - d->timediff;
			latency = correctedNow-newPos.timestamp;
			d->timediff += latency/10.0;
			d->meanLatency = (d->meanLatency*9.0 + ((latency<0)?-latency:latency))/10.0;
		}
		dout << "timediff=" << d->timediff << " meanLat=" << d->meanLatency << " latency=" << latency << std::endl;
					   
		// make new position active
		EnterCriticalSection( &d->cs );
		if( !d->extrapolate ) {
			d->extrapolate = true;
			for( int x=0; x<3; x++ ) {
				d->posVel[x] = 0;
				d->dirVel[x] = 0;
			}
			d->reference = newPos;
			d->extrapolated = newPos;
			dout << "First position packet " << to_string(newPos.pos) << std::endl;
		} else { //if( abs(latency-d->meanLatency)<5*abs(d->meanLatency) ) {
			// calculate moved distance
			double dlat = x_to_lat(newPos.pos[0]-d->reference.pos[0]);
			double dlon = y_to_lon(newPos.pos[1]-d->reference.pos[1]);
			double dalt = z_to_m(newPos.pos[2]-d->reference.pos[2]);
			double dist = sqrt( sqr( dlat_to_m(dlat) )
								+ sqr( dlon_to_m(dlon,x_to_lat(newPos.pos[0])) )
								+ sqr( dalt ) );
			double vel = dist/(newPos.timestamp-d->reference.timestamp);

			// reasonable speed?
			if( vel*3.6>10000 ) {
				// jumped plane, reset position
				for( int x=0; x<3; x++ ) {
					d->posVel[x] = 0;
					d->dirVel[x] = 0;
				}
				d->reference = newPos;
				d->extrapolated = newPos;
				dout << "Jump (" << dist/1000.0 << "km)" << " to " << to_string(newPos.pos) << std::endl;
			} else {
				// calculate speeds:
				// airplane speed 
				// + difference between extrapolation and newPos position
				double dt = (newPos.timestamp-d->reference.timestamp);
				for( int x=0; x<3; x++ ) {
					// position speed
					d->posVel[x] = 
						(newPos.pos[x]-d->reference.pos[x])/dt
						+ (newPos.pos[x]-d->extrapolated.pos[x]
						   +(newPos.pos[x]-d->reference.pos[x])/dt*latency);
										
					// direction speed
					double ddir = dir_to_deg(newPos.dir[x])
						- dir_to_deg(d->reference.dir[x]);
					if( ddir>180.0 ) ddir -= 360.0;
					if( ddir<=-180.0 ) ddir += 360.0;
					d->dirVel[x] = 
						ddir/dt
						+ (dir_to_deg(newPos.dir[x])+ddir/dt*latency
						   - dir_to_deg(d->extrapolated.dir[x]));				   
				}        
				dout << "Move (" << dist << "m) from " << to_string(d->extrapolated.pos) 
					 << " (with " << to_string(d->posVel) 
					 << " ) to " << to_string(newPos.pos)
					 << std::endl;
				/*dout << "Turn from " << to_string(d->extrapolated.dir) 
				  << " (with " << to_string(d->dirVel) 
				  << " ) to " << to_string(newPos.dir)
				  << std::endl;*/
				
				d->reference = newPos;
				d->lastReferenceTime = now;
			}
		}
		LeaveCriticalSection( &d->cs );
	} break;
	}
}
