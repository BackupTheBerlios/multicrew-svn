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


class Element : public PacketFactory<PacketBase> {
public:
	Element( int id, Gauge &gauge );
	virtual ~Element();

	ELEMENT_HEADER *elementHeader();
	Gauge &gauge();
	int id();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void sendProc() {};
	virtual void receive( SmartPtr<PacketBase> packet ) {}
	virtual void detach();
	
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer )=0;

 protected:
	MulticrewCore *core();
	
 private:
	struct Data;
	Data *d;
};


class IconElement : public Element {
 public:
	IconElement( int id, Gauge &gauge );
	virtual ~IconElement();

	ELEMENT_ICON *iconHeader();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

 private:
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual void sendProc();
	virtual FLOAT64 callback( PELEMENT_ICON pelement );

	struct Data;
	friend Data;
	Data *d;
};


class NeedleElement : public Element {
 public:
	NeedleElement( int id, Gauge &gauge );
	virtual ~NeedleElement();

	ELEMENT_NEEDLE *needleHeader();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

 private:
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual void sendProc();
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );

	struct Data;
	friend Data;
	Data *d;
};


class StringElement : public Element {
 public:
	StringElement( int id, Gauge &gauge );
	virtual ~StringElement();

	ELEMENT_STRING *stringHeader();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();
	
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

 private:
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
	virtual void sendProc();
	virtual void receive( SmartPtr<PacketBase> );

	struct Data;
	friend Data;
	Data *d;
};


class StaticElement : public Element {
public:
	StaticElement( int id, Gauge &gauge );
	virtual ~StaticElement();

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer ) { return 0; }

protected:	
	struct Data;
	Data *d;
};


class Gauge : public PacketFactory<PacketBase> {
public:
	Gauge( GaugeModule *mgauge, int id );
	virtual ~Gauge();

	PGAUGEHDR gaugeHeader() ;
	std::string name();
	std::string parameter();
	int id();
	GaugeModule *mgauge();
	virtual void sendProc( bool fullSend );

	void send( unsigned element, SmartPtr<PacketBase> packet, 
			   bool safe, bool async=false );
	virtual void receive( SmartPtr<PacketBase> packet );
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

	virtual void attach( PGAUGEHDR gaugeHeader );
	virtual void detach();

	void requestSend( Element *element );

 protected:	
	void createElements();
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	Element *createElement( int id, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
	virtual void boostMetafileThread( HWND, UINT, UINT_PTR, DWORD) {}
	void handleMouseEvents();

	bool configBoolValue( const std::string &key, bool def );
	int configIntValue( const std::string &key, int def );

	struct Data;
	friend Data;
	Data *d;
};


class MetafileGauge : public Gauge {
 public:
	MetafileGauge( GaugeModule *mgauge, int id, int metafileElement,
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

	void send( unsigned gauge, SmartPtr<PacketBase> packet, bool safe, 
			   Connection::Priority prio=Connection::mediumPriority, 
			   bool async=false );

	void sendMetafilePacket( unsigned channel, SmartPtr<PacketBase> packet, bool safe, 
							 Connection::Priority prio=Connection::mediumPriority );

	PGAUGE_CALLBACK installCallback();

	void requestSend( Gauge *gauge );
	virtual void sendFullState();
	
 protected:
	friend Gauge;
	void detached( Gauge *gauge );
	virtual SmartPtr<PacketBase> createInnerModulePacket( SharedBuffer &buffer );

	bool configBoolValue( PGAUGEHDR pgauge, const std::string &key, bool def );
	int configIntValue( PGAUGEHDR pgauge, const std::string &key, int def );

 protected:	
	virtual void handlePacket( SmartPtr<PacketBase> packet );
	void installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual void sendProc();

	Gauge *getDetachedGauge( const char *name, const char *parameter );
	
 private:
	struct Data;
	friend Data;
	Data *d;
};
