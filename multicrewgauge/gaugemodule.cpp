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


/****************************************************************************/
typedef TypedPacket<char,PacketBase> MetafileChannelPacket;


class MetafileChannelImpl : public MetafileChannel,
							public Identified,
							private NetworkChannel, 
							private TypedPacketFactory<char,PacketBase> {
 public:
	MetafileChannelImpl( GaugeModule *mgauge, unsigned num )
		: Identified( mgauge, num ), 
		  NetworkChannel( id() ),
		  num( num ),
		  compressor( this, 
					  mgauge->config()->intValue( 
						  "metafile", 
						  "delay"+to_string(num),
						  500 ) ),
		  decompressor( this ) {
	}

	unsigned channelNum() {
		return num;
	}

	MetafileCompressor compressor;
	MetafileDecompressor decompressor;

 private:
	bool send( bool fromCompressor, SmartPtr<PacketBase> packet, 
			   bool safe, Priority prio ) {
		return NetworkChannel::send( 
			new MetafileChannelPacket( fromCompressor?1:0, packet ),
			safe, prio );
	}

	SmartPtr<PacketBase> createPacket( char key, SharedBuffer &buffer ) {
		if( key==0 )
			return compressor.createPacket( buffer );
		else
			return decompressor.createPacket( buffer );
	}

	void receive( SmartPtr<PacketBase> packet ) {
		SmartPtr<MetafileChannelPacket> mcp = (MetafileChannelPacket*)&*packet;
		if( mcp->key()==0 )
			compressor.receive( mcp->wrappee() );
		else
			decompressor.receive( mcp->wrappee() );
	}

	SmartPtr<PacketBase> createPacket( SharedBuffer &buffer ) {
		return new MetafileChannelPacket( buffer, this );
	}

	void sendFullState() {
	}

	unsigned num;
};


typedef SmartPtr<MetafileChannelImpl> SmartMetafileChannelImpl;


/****************************************************************************/
typedef std::list<Gauge*> GaugeList;
struct GaugeModule::Data {
	Data( GaugeModule *mg )
		: installCallbackAdapter( mg, GaugeModule::installGauge ) {}

	VoidCallbackAdapter3<GaugeModule, PGAUGEHDR, SINT32, UINT32> installCallbackAdapter;

	SmartPtr<MulticrewCore> core;
	std::deque<Gauge*> gauges;
	std::deque<SmartMetafileChannelImpl> metafileChannels;
	std::map<std::string, GaugeList*> detachedGauges;
};


GaugeModule::GaugeModule( std::string moduleName )
	: MulticrewModule( moduleName ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();
}


GaugeModule::~GaugeModule() {
	// delete gauges
	//lock();
	std::deque<Gauge*>::iterator it = d->gauges.begin();
	while( it!=d->gauges.end() ) delete *(it++);
	d->gauges.clear();
	//unlock();

	// clear metafile channels
	d->metafileChannels.clear();

	delete d;
}


bool GaugeModule::configBoolValue( PGAUGEHDR pgauge, const std::string &key, bool def ) {
	// first look in [name!parameter], then in [name], then def
	std::string name = pgauge->gauge_name;
	std::string parameters;
	if( pgauge->parameters )
		parameters = pgauge->parameters;

	return config()->boolValue( name+"!"+parameters, key,
								config()->boolValue( name, key, def ) );
}


int GaugeModule::configIntValue( PGAUGEHDR pgauge, const std::string &key, int def ) {
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
void GaugeModule::installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call old callback
	if( (int)(pgauge->user_area[9])!=0 )
		((PGAUGE_CALLBACK)(int)(pgauge->user_area[9]))( pgauge, service_id, extra_data );

	if( service_id==PANEL_SERVICE_POST_INSTALL ) {
		// restore old callback
		pgauge->gauge_callback = (PGAUGE_CALLBACK)(int)(pgauge->user_area[9]);

		//lock();
		// look for detached gauge which is to be reused
		Gauge *gauge = getDetachedGauge( pgauge->gauge_name, pgauge->parameters );
		std::string name( pgauge->gauge_name );	
		std::string parameters( (pgauge->parameters!=NULL)?pgauge->parameters:"" );

		if( gauge==0 ) {
			dout << "new "
				 << pgauge << " "
				 << name << "§" << parameters 
				 << " num=" << d->gauges.size()
				 << " user_area[9]=" 
				 << (void*)(int)pgauge->user_area[9] << std::endl;

			// no detached gauge found, create new one
			int metafile = configIntValue( pgauge, "metafile", -1 );
			int element = configIntValue( pgauge, "metafileelement", 0 );
			
			// create new gauge
			if( metafile>=0 ) {
				// create enough (de)compression channels
				while( metafile>=d->metafileChannels.size() ) {
					unsigned num = d->metafileChannels.size();
					d->metafileChannels.push_back( 
						new MetafileChannelImpl( this, num ) );
				}
						
				// create the gauge with the associated metafile channel
				gauge = new MetafileGauge( 
					this, name, parameters,
					element,
					&d->metafileChannels[metafile]->compressor,
					&d->metafileChannels[metafile]->decompressor );
			} else
				gauge = new Gauge( this, name, parameters );

			d->gauges.push_back( gauge );
		} else {
			dout << "reusing "
				 << pgauge << " "
				 << id() << "/"
				 << name << "§" << parameters
				 << " num=" << gauge->id()
				 << " user_area[9]=" 
				 << (void*)(int)pgauge->user_area[9] << std::endl;			
		}
		gauge->attach( pgauge ); 
		//unlock();
	}
}


PGAUGE_CALLBACK GaugeModule::installCallback() {
	return d->installCallbackAdapter.callback();
}


Gauge *GaugeModule::getDetachedGauge( const char *name, 
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


void GaugeModule::detached( Gauge *gauge ) {
	std::string detachId = gauge->name() + "§" + gauge->parameter();
	dout << "detaching " << detachId << std::endl;

	// find detached gauges with this name and parameter
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
