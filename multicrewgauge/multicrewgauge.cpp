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
#include "../multicrewcore/packets.h"


#define WAITTIME 500


typedef TypedPacket<unsigned, PacketBase> RoutedPacket;


class RoutedPacketFactory : public TypedPacketFactory<unsigned,PacketBase> {
public:
	RoutedPacketFactory( std::deque<Gauge *> &gauges, 
						 std::deque<SmartMetafileChannel> &metafileChannels ) 
		: _gauges( gauges ),
		  _metafileChannels( metafileChannels ) {
	}

	virtual SmartPtr<PacketBase> createPacket( unsigned key, SharedBuffer &buffer ) {
		// metafile or gauge packet?
		if( key>=1000000 ) {
			unsigned channel = key-1000000;
			if( channel<_metafileChannels.size() )
				return _metafileChannels[channel]->createPacket( buffer );
			else
				return 0;
		} else {
			if( key<_gauges.size() )
				return _gauges[key]->createPacket( buffer );
			else
				return 0;
		}
	}

private:
	std::deque<Gauge *> &_gauges;
	std::deque<SmartMetafileChannel> &_metafileChannels;
};



typedef std::list<Gauge*> GaugeList;
struct MulticrewGauge::Data {
	Data( MulticrewGauge *mg )
		: installCallbackAdapter( mg, MulticrewGauge::installGauge ),
		  packetFactory( gauges, metafileChannels ) {
	}

	virtual ~Data() {
	}

	VoidCallbackAdapter3<MulticrewGauge, PGAUGEHDR, SINT32, UINT32> installCallbackAdapter;

	SmartPtr<MulticrewCore> core;
	std::deque<Gauge*> gauges;
	std::deque<SmartMetafileChannel> metafileChannels;
	std::map<std::string, GaugeList*> detachedGauges;
	RoutedPacketFactory packetFactory;
	std::set<Gauge*> sendRequests;
	bool nextIsFullSend;
};


MulticrewGauge::MulticrewGauge( bool hostMode, std::string moduleName )
	: MulticrewModule( moduleName, hostMode, WAITTIME ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();
	d->nextIsFullSend = false;
}


MulticrewGauge::~MulticrewGauge() {
	// stop the parent class thread which might call stepProc otherwise
	// which depends on this->d
	disconnect();

	// delete gauges
	lock();
	std::deque<Gauge*>::iterator it = d->gauges.begin();
	while( it!=d->gauges.end() ) delete *(it++);
	d->gauges.clear();
	unlock();

	// clear metafile channels
	d->metafileChannels.clear();

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


bool MulticrewGauge::configBoolValue( PGAUGEHDR pgauge, const std::string &key, bool def ) {
	// first look in [name!parameter], then in [name], then def
	std::string name = pgauge->gauge_name;
	std::string parameters;
	if( pgauge->parameters )
		parameters = pgauge->parameters;

	return config()->boolValue( name+"!"+parameters, key,
								config()->boolValue( name, key, def ) );
}


int MulticrewGauge::configIntValue( PGAUGEHDR pgauge, const std::string &key, int def ) {
	// first look in [name!parameter], then in [name], then def
	std::string name = pgauge->gauge_name;
	std::string parameters;
	if( pgauge->parameters )
		parameters = pgauge->parameters;

	return config()->intValue( name+"!"+parameters, key,
						config()->intValue( name, key, def ) );
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
		/*if( pgauge->parameters!=NULL && strlen(pgauge->parameters)>0 )
			dout << "New gauge " << pgauge << " " << pgauge->gauge_name 
				 <<	" with " << pgauge->parameters << " id=" << d->gauges.size() << std::endl;
		else
		dout << "New gauge " << pgauge << " " << pgauge->gauge_name << " id=" << d->gauges.size() << std::endl;*/

		// restore old callback
		pgauge->gauge_callback = (PGAUGE_CALLBACK)(int)(pgauge->user_area[9]);

		lock();
		// look for detached gauge which is to be reused
		Gauge *gauge = getDetachedGauge( pgauge->gauge_name, pgauge->parameters );
		if( gauge==0 ) {
			int metafile = configIntValue( pgauge, "metafile", -1 );
			int element = configIntValue( pgauge, "metafileelement", 0 );
			
			// create new gauge
			if( isHostMode() )
				if( metafile>=0 ) {
					// create enough compression channels
					while( metafile>=d->metafileChannels.size() ) {
						int delay = 300; //configIntValue( 300; //config()->intValue( "metafile", 
						d->metafileChannels.push_back( 
							new MetafileCompressor( this, delay,
													d->metafileChannels.size() ));
					}
						
					// create the gauge with the associated metafile channel
					gauge = new GaugeMetafileRecorder( 
						this, d->gauges.size(), 
						element,
						(MetafileCompressor*)&*d->metafileChannels[metafile] );
				} else
					gauge = new GaugeRecorder( this, d->gauges.size() );
			else
				if( metafile>=0 ) {
					// create enough decompression channels
					while( metafile>=d->metafileChannels.size() ) {
						d->metafileChannels.push_back( 
							new MetafileDecompressor( this, 
													  d->metafileChannels.size() ));
					}

					// create the gauge with the associated metafile channel
					gauge = new GaugeMetafileViewer( 
						this, d->gauges.size(), 
						element, 
						(MetafileDecompressor*)&*d->metafileChannels[metafile] );
				} else
					gauge = new GaugeViewer( this, d->gauges.size() );
			
			d->gauges.push_back( gauge );
		}
		gauge->attach( pgauge ); 
		unlock();
	}
}


void MulticrewGauge::handlePacket( SmartPtr<PacketBase> packet ) {
	SmartPtr<RoutedPacket> gp = (RoutedPacket*)&*packet;
	lock();
	
	// metafile or gauge packet?
	if( gp->key()>=1000000 ) {
		// send to metafile channel
		unsigned channel = gp->key()-1000000;
		if( channel<d->metafileChannels.size() ) {
			unlock();
			d->metafileChannels[channel]->receive( gp->wrappee() );
		} else
			unlock();
	} else {
		// send to gauge
		if( gp->key()>=0 && gp->key()<d->gauges.size() ) {
			Gauge *gauge = d->gauges[gp->key()];
			unlock();
			gauge->receive( (RoutedPacket*)&*gp->wrappee() );
		} else
			unlock();
	}
}


void MulticrewGauge::send( unsigned gauge, SmartPtr<PacketBase> packet, bool safe,
						   Connection::Priority prio, bool async ) {
	if( async )
		MulticrewModule::sendAsync( 
			new RoutedPacket(gauge, packet), 
			safe,
			prio );
	else
		MulticrewModule::send( 
			new RoutedPacket(gauge, packet), 
			safe,
			prio );
}


void MulticrewGauge::sendMetafilePacket( unsigned channel, 
										 SmartPtr<PacketBase> packet, 
										 bool safe,
										 Connection::Priority prio ) {
	MulticrewModule::sendAsync( 
		new RoutedPacket(1000000+channel, packet), 
		safe,
		prio );
}


void MulticrewGauge::requestSend( Gauge *gauge ) {
	lock();
	d->sendRequests.insert( gauge );
	unlock();
}


void MulticrewGauge::sendProc() {
	lock();

	if( d->nextIsFullSend ) {
		d->nextIsFullSend = false;

		// send data of all gauges
		for( std::deque<Gauge*>::iterator it = d->gauges.begin();
			 it!=d->gauges.end();
			 it++ ) {
			(*it)->sendProc( true );
		}
	} else {
		// call send proc for all gauges in the sendRequests queue
		for( std::set<Gauge*>::iterator it = d->sendRequests.begin();
			 it!=d->sendRequests.end();
			 it++ )
			(*it)->sendProc( false );
	}
	
	// clear send queue in any case
	d->sendRequests.clear();
		
	unlock();
}


void MulticrewGauge::sendFullState() {
	d->nextIsFullSend = true;
}


PGAUGE_CALLBACK MulticrewGauge::installCallback() {
	return d->installCallbackAdapter.callback();
}


Gauge *MulticrewGauge::getDetachedGauge( const char *name, 
										 const char *parameter ) {
	// find detached gauges with this name and paramter
	std::string detachId = std::string(name) + "§" + std::string((parameter!=NULL)?parameter:"");
	std::map<std::string, GaugeList*>::iterator it = d->detachedGauges.find( detachId );
	if( it==d->detachedGauges.end() ) return 0;

	// return the first (=oldest) of those gauges
	GaugeList *list = it->second;
	if( list->empty() ) return 0;
	Gauge *ret = list->front();
	list->pop_front();
	return ret;
}


void MulticrewGauge::detached( Gauge *gauge ) {
	std::string detachId = gauge->name() + "§" + gauge->parameter();
	dout << "detaching " << detachId << std::endl;

	// find detached gauges with this name and paramter
	std::map<std::string, GaugeList*>::iterator it = d->detachedGauges.find( detachId );
	GaugeList *list;
	if( it!=d->detachedGauges.end() )
		list = it->second;
	else {
		list = new GaugeList();
		d->detachedGauges[detachId] = list;
	}
			
	// add gauge to list
	list->push_back( gauge );
}


SmartPtr<PacketBase> MulticrewGauge::createInnerModulePacket( SharedBuffer &buffer ) {
	lock();
	SmartPtr<RoutedPacket> gp = new RoutedPacket( buffer, &d->packetFactory );
	unlock();
	return gp;
}
