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
#include "../multicrewcore/debug.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/shared.h"
#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/error.h"
#include "../multicrewcore/config.h"
#include "../multicrewcore/log.h"


struct MulticrewGauge::Data {
	Data( MulticrewGauge *mg )
		: installCallbackAdapter( mg, MulticrewGauge::installGauge ) {
	}

	VoidCallbackAdapter3<MulticrewGauge, PGAUGEHDR, SINT32, UINT32> installCallbackAdapter;

	SmartPtr<MulticrewCore> core;
	int gaugeCounter;
	bool registered;
	std::deque<Gauge*> gauges;
};

MulticrewGauge::MulticrewGauge( bool hostMode, std::string moduleName )
	: MulticrewModule( moduleName, hostMode ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();
	d->gaugeCounter = 0;
	d->registered = false;
}


MulticrewGauge::~MulticrewGauge() {
	// delete all gauges
	std::deque<Gauge*>::iterator it = d->gauges.begin();
	while( it!=d->gauges.end() ) delete *(it++);
	d->gauges.clear();	

	// unregister from core
	if( d->registered )
		d->core->unregisterModule( this );
	delete d;
}


bool MulticrewGauge::init() {
	// register with core
	bool ret = d->core->registerModule( this );
	if( !ret ) {
		dout << "MulticrewGauge init failed" << std::endl;
		return false;
	}

	d->registered = true;
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
		dout << "New gauge " << pgauge << " " << pgauge->gauge_name;
		if( pgauge->parameters!=NULL && strlen(pgauge->parameters)>0 ) dout << " with " << pgauge->parameters;
		dout << std::endl;

		// restore old callback
		pgauge->gauge_callback = (PGAUGE_CALLBACK)(int)(pgauge->user_area[9]);

		// register gauge
		if( isHostMode() )
			d->gauges.push_back( new GaugeRecorder( this, d->gaugeCounter, pgauge ) ); 
		else
			d->gauges.push_back( new GaugeViewer( this, d->gaugeCounter, pgauge ) );
		d->gaugeCounter++;
	}
}


void MulticrewGauge::receive( ModulePacket *packet ) {
	if( packet->id>=firstUpdatePacket ) {
		UpdatePacket *up = (UpdatePacket *)packet;
		if( up->gauge>=0 && up->gauge<d->gauges.size() )
			d->gauges[up->gauge]->receive( up );
	}
}


void MulticrewGauge::send( UpdatePacket *packet, bool safe ) {
	MulticrewModule::send( packet, safe, false );
}


PGAUGE_CALLBACK MulticrewGauge::installCallback() {
	return d->installCallbackAdapter.callback();
}
