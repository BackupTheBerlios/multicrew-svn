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

#include <deque>
#include <string>

#include "gauges.h"
#include "metafile.h"
#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/config.h"
#include "../multicrewcore/thread.h"
#include "zdelta/zdlib.h"


// general
extern GAUGEHDR gaugehdr_multicrewgauge;
extern SmartPtr<MulticrewCore> core;

// classes
class Element;
class IconElement;
class Gauge;
class GaugeModule;


class Element : public Identified {
public:
	Element( Gauge *gauge, unsigned num );
	virtual ~Element();

	ELEMENT_HEADER *elementHeader();
	Gauge *gauge();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void receive( SmartPtr<PacketBase> packet ) {}
	virtual void detach();

 protected:
	MulticrewCore *core();
	
 private:
	struct Data;
	Data *d;
};


class IconElement : public Element, private NetworkChannel {
 public:
	IconElement( Gauge *gauge, unsigned num );
	virtual ~IconElement();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();

 private:
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );	
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual void sendFullState();
	void sendState();

	virtual FLOAT64 callback( PELEMENT_ICON pelement );

	struct Data;
	friend Data;
	Data *d;
};


class NeedleElement : public Element, public NetworkChannel {
 public:
	NeedleElement( Gauge *gauge, unsigned num );
	virtual ~NeedleElement();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();

 private:
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );	
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual void sendFullState();
	void sendState();

	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );

	struct Data;
	friend Data;
	Data *d;
};


class StringElement : public Element, public NetworkChannel {
 public:
	StringElement( Gauge *gauge, unsigned num );
	virtual ~StringElement();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();
	
 private:
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );	
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual void sendFullState();
	void sendState();

	virtual FLOAT64 callback( PELEMENT_STRING pelement );

	struct Data;
	friend Data;
	Data *d;
};


class StaticElement : public Element {
public:
	StaticElement( Gauge *gauge, unsigned num );
	virtual ~StaticElement();
};


class Gauge : private NetworkChannel, public Identified {
 public:
	Gauge( GaugeModule *mgauge, std::string name, std::string parameters );
	virtual ~Gauge();

	PGAUGEHDR gaugeHeader() ;
	std::string name();
	std::string parameter();
	GaugeModule *mgauge();

	virtual void attach( PGAUGEHDR gaugeHeader );
	virtual void detach();

 protected:	
	void createElements();
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	Element *createElement( int num, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
	virtual void boostMetafileThread( HWND, UINT, UINT_PTR, DWORD) {}
	void handleMouseEvents();

	void receive( SmartPtr<PacketBase> packet );
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );
	void sendFullState();

	bool configBoolValue( const std::string &key, bool def );
	int configIntValue( const std::string &key, int def );

	struct Data;
	friend Data;
	Data *d;
};


class MetafileGauge : public Gauge {
 public:
	MetafileGauge( GaugeModule *mgauge, 
				   std::string name, std::string parameter,
				   int metafileElement,
				   SmartPtr<MetafileCompressor> compressor,
				   SmartPtr<MetafileDecompressor> decompressor );
	virtual ~MetafileGauge();

 private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );

	struct Data;
	friend Data;
	Data *md;
};


class GaugeModule : public MulticrewModule {
 public:
	GaugeModule( std::string moduleName );
	virtual ~GaugeModule();

	PGAUGE_CALLBACK installCallback();

 protected:
	friend Gauge;
	void detached( Gauge *gauge );
	bool configBoolValue( PGAUGEHDR pgauge, const std::string &key, bool def );
	int configIntValue( PGAUGEHDR pgauge, const std::string &key, int def );

	void installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	Gauge *getDetachedGauge( const char *name, const char *parameter );
	
 private:
	struct Data;
	friend Data;
	Data *d;
};
