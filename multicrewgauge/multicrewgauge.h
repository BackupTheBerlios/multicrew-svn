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
extern HMODULE gInstance;
extern SmartPtr<MulticrewCore> core;

// classes
class Element;
class IconElement;
class Gauge;
class MulticrewGauge;


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
	
protected:
	virtual FLOAT64 callback( PELEMENT_ICON pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class IconRecorder : public IconElement {
public:
	IconRecorder( int id, Gauge &gauge );
	virtual ~IconRecorder();

	virtual void sendProc();

private:
	virtual FLOAT64 callback( PELEMENT_ICON pelement );
};


class IconViewer : public IconElement {
public:
	IconViewer( int id, Gauge &gauge );
	virtual ~IconViewer();

	virtual void receive( SmartPtr<PacketBase> packet );

private:	
	virtual FLOAT64 callback( PELEMENT_ICON pelement );
};


class NeedleElement : public Element {
public:
	NeedleElement( int id, Gauge &gauge );
	virtual ~NeedleElement();

	ELEMENT_NEEDLE *needleHeader();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );
	
protected:
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class NeedleRecorder : public NeedleElement {
public:
	NeedleRecorder( int id, Gauge &gauge );
	virtual ~NeedleRecorder();

	virtual void sendProc();

private:	
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );
};


class NeedleViewer : public NeedleElement {
public:
	NeedleViewer( int id, Gauge &gauge );
	virtual ~NeedleViewer();

	virtual void receive( SmartPtr<PacketBase> packet );

private:	
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );
};


class StringElement : public Element {
public:
	StringElement( int id, Gauge &gauge );
	virtual ~StringElement();

	ELEMENT_STRING *stringHeader();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void detach();
	
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

protected:
	virtual FLOAT64 callback( PELEMENT_STRING pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class StringRecorder : public StringElement {
public:
	StringRecorder( int id, Gauge &gauge );
	virtual ~StringRecorder();

	virtual void sendProc();

private:
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
};


class StringViewer : public StringElement {
public:
	StringViewer( int id, Gauge &gauge );
	virtual ~StringViewer();

	virtual void receive( SmartPtr<PacketBase> );

private:	
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
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


class StaticRecorder : public StaticElement {
public:
	StaticRecorder( int id, Gauge &gauge );
	virtual ~StaticRecorder();
};


class StaticViewer : public StaticElement {
public:
	StaticViewer( int id, Gauge &gauge );
	virtual ~StaticViewer();
};


class Gauge : public PacketFactory<PacketBase> {
public:
	Gauge( MulticrewGauge *mgauge, int id );
	virtual ~Gauge();

	PGAUGEHDR gaugeHeader() ;
	std::string name();
	std::string parameter();
	int id();
	MulticrewGauge *mgauge();
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
	virtual Element *createElement( int id, PELEMENT_HEADER pelement )=0;
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags )=0;
	virtual void boostMetafileThread( HWND, UINT, UINT_PTR, DWORD) {}

	bool configBoolValue( const std::string &key, bool def );
	int configIntValue( const std::string &key, int def );

	struct Data;
	friend Data;
	Data *d;
};


class GaugeRecorder : public Gauge, public AsyncCallee {
public:
	GaugeRecorder( MulticrewGauge *mgauge, int id );
	virtual ~GaugeRecorder();
	virtual void receive( SmartPtr<PacketBase> packet );

 private:
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
	virtual void asyncCallback();
};


class GaugeMetafileRecorder : public GaugeRecorder {
public:
	GaugeMetafileRecorder( MulticrewGauge *mgauge, int id, int metafileElement,
						   SmartPtr<MetafileCompressor> compressor );
	virtual ~GaugeMetafileRecorder();

private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	SmartPtr<MetafileCompressor> compressor;
};


class GaugeViewer : public Gauge  {
public:
	GaugeViewer( MulticrewGauge *mgauge, int id );
	virtual ~GaugeViewer();

private:
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
};


class GaugeMetafileViewer : public GaugeViewer  {
public:
	GaugeMetafileViewer( MulticrewGauge *mgauge, int id, int metafileElement,
						 SmartPtr<MetafileDecompressor> decompressor );
	virtual ~GaugeMetafileViewer();

private:
	SmartPtr<MetafileDecompressor> decompressor;
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
};


class MulticrewGauge : public MulticrewModule {
 public:
	MulticrewGauge( bool hostMode, std::string clientName );
	virtual ~MulticrewGauge();
	bool init();

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
