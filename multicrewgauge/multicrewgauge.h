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
#include "../multicrewcore/multicrewcore.h"


// general
extern GAUGEHDR gaugehdr_multicrewgauge;
extern HMODULE gInstance;
extern SmartPtr<MulticrewCore> core;

// classes
class Element;
class IconElement;
class Gauge;
class MulticrewGauge;


class Element : public PacketFactory<Packet> {
public:
	Element( int id, Gauge &gauge );
	virtual ~Element();

	ELEMENT_HEADER *elementHeader();
	Gauge &gauge();
	int id();

	virtual void attach( ELEMENT_HEADER *elementHeader );
	virtual void sendProc() {};
	virtual void receive( SmartPtr<Packet> packet ) {}
	virtual void detach();
	
	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer )=0;

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

	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer );
	
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

	virtual void receive( SmartPtr<Packet> packet );

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

	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer );
	
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

	virtual void receive( SmartPtr<Packet> packet );

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
	
	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer );

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

	virtual void receive( SmartPtr<Packet> );

private:	
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
};


class StaticElement : public Element {
public:
	StaticElement( int id, Gauge &gauge );
	virtual ~StaticElement();

	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer ) { return 0; }

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


class Gauge : public PacketFactory<Packet> {
public:
	Gauge( MulticrewGauge *mgauge, int id );
	virtual ~Gauge();

	PGAUGEHDR gaugeHeader() ;
	std::string name();
	std::string parameter();
	int id();
	MulticrewGauge *mgauge();
	virtual void sendProc();

	void send( unsigned element, SmartPtr<Packet> packet, bool safe );
	virtual void receive( SmartPtr<Packet> packet );
	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer );

	virtual void attach( PGAUGEHDR gaugeHeader );
	virtual void detach();

 protected:	
	void createElements();
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual Element *createElement( int id, PELEMENT_HEADER pelement )=0;
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags )=0;

	struct Data;
	friend Data;
	Data *d;
};


class GaugeRecorder : public Gauge {
public:
	GaugeRecorder( MulticrewGauge *mgauge, int id );
	virtual ~GaugeRecorder();
	virtual void receive( SmartPtr<Packet> packet );

private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
};


class GaugeViewer : public Gauge  {
public:
	GaugeViewer( MulticrewGauge *mgauge, int id );
	virtual ~GaugeViewer();
	virtual void sendProc();

private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
	virtual BOOL mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags );
};

class MulticrewGauge : public MulticrewModule {
 public:
	MulticrewGauge( bool hostMode, std::string clientName );
	virtual ~MulticrewGauge();
	bool init();

	void send( unsigned gauge, SmartPtr<Packet> packet, bool safe );
	PGAUGE_CALLBACK installCallback();
	virtual SmartPtr<Packet> createPacket( SharedBuffer &buffer );
	
 protected:
	friend Gauge;
	void detached( Gauge *gauge );

 protected:	
	virtual void handlePacket( SmartPtr<Packet> packet );
	void installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual void sendProc();

	Gauge *getDetachedGauge( const char *name, const char *parameter );
	
 private:
	struct Data;
	friend Data;
	Data *d;
};
