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
#include <string>
#include <deque>
#include <list>

#include "gauges.h"
#include "multicrewgauge.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/shared.h"
#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/config.h"
#include "../multicrewcore/log.h"


#define WAITTIME 5

enum PacketType {
	gaugePacket=0,
	eventPacket,
};

#pragma pack(push,1)
struct BasePacket {	
	PacketType type;
};

struct GaugePacket : public BasePacket {
	int gauge;
	char data[0];
};

struct EventPacket : public BasePacket {
	ID id;
};
#pragma pack(pop,1)


struct MulticrewGauge::Data {
	Data( MulticrewGauge *mg )
		: installCallbackAdapter( mg, MulticrewGauge::installGauge ) {
	}

	VoidCallbackAdapter3<MulticrewGauge, PGAUGEHDR, SINT32, UINT32> installCallbackAdapter;

	SmartPtr<MulticrewCore> core;
	int gaugeCounter;
	std::deque<Gauge*> gauges;

	CRITICAL_SECTION cs;
};

MulticrewGauge::MulticrewGauge( bool hostMode, std::string moduleName )
	: MulticrewModule( moduleName, hostMode, WAITTIME ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();
	d->gaugeCounter = 0;
	InitializeCriticalSection( &d->cs );
}


MulticrewGauge::~MulticrewGauge() {
	// stop the parent class thread which might call stepProc otherwise
	// which depends on this->d
	disconnect();

	// delete all gauges
	EnterCriticalSection( &d->cs );
	std::deque<Gauge*>::iterator it = d->gauges.begin();
	while( it!=d->gauges.end() ) delete *(it++);
	d->gauges.clear();
	LeaveCriticalSection( &d->cs );

	DeleteCriticalSection( &d->cs );
	delete d;
}


bool MulticrewGauge::init() {
	// register with core
	if( !registered() ) {
		dout << "MulticrewGauge init failed" << std::endl;
		return false;
	}

	return true;
}


/*
 * Register gauge by creating a Gauge object. But not before
 * PREINSTALL, because the parameter isn't before
 */
void MulticrewGauge::installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call old callback
	if( (int)(pgauge->user_area[9])!=0 )
		((PGAUGE_CALLBACK)(int)(pgauge->user_area[9]))( pgauge, service_id, extra_data );	

	if( service_id==PANEL_SERVICE_POST_INSTALL ) {
		if( pgauge->parameters!=NULL && strlen(pgauge->parameters)>0 )
			dout << "New gauge " << pgauge << " " << pgauge->gauge_name 
				 <<	" with " << pgauge->parameters << std::endl;
		else
			dout << "New gauge " << pgauge << " " << pgauge->gauge_name << std::endl;

		// restore old callback
		pgauge->gauge_callback = (PGAUGE_CALLBACK)(int)(pgauge->user_area[9]);

		// register gauge
		EnterCriticalSection( &d->cs );
		if( isHostMode() )
			d->gauges.push_back( new GaugeRecorder( this, d->gaugeCounter, pgauge ) ); 
		else
			d->gauges.push_back( new GaugeViewer( this, d->gaugeCounter, pgauge ) );
		LeaveCriticalSection( &d->cs );
		d->gaugeCounter++;
	}
}


void MulticrewGauge::receive( void *data, unsigned size ) {
	BasePacket *p = (BasePacket*)data;
	switch( p->type ) {
	case gaugePacket: 
	{
		GaugePacket *gp = (GaugePacket*)p;
		EnterCriticalSection( &d->cs );
		if( gp->gauge>=0 && gp->gauge<d->gauges.size() ) {
			Gauge *gauge = d->gauges[gp->gauge];
			LeaveCriticalSection( &d->cs );
			gauge->receive( gp->data, size-sizeof(GaugePacket) );
		} else {
			LeaveCriticalSection( &d->cs );
		}
	}
	break;
	default:
		break;
	}
}


void MulticrewGauge::send( int gauge, void *data, unsigned size, bool safe, bool append ) {
	if( !append ) {
		// add route
		GaugePacket p;
		p.type = gaugePacket;
		p.gauge = gauge;
		MulticrewModule::send( &p, sizeof(GaugePacket), safe, Connection::MediumPriority );
	}
	
	// add data
	MulticrewModule::send( data, size, safe, Connection::MediumPriority, true );
}


void MulticrewGauge::sendProc() {
	EnterCriticalSection( &d->cs );
	for( int i=0; i<d->gauges.size(); i++ )
		d->gauges[i]->sendProc();
	LeaveCriticalSection( &d->cs );
}


PGAUGE_CALLBACK MulticrewGauge::installCallback() {
	return d->installCallbackAdapter.callback();
}
