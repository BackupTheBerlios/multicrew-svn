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
#include "../multicrewcore/packets.h"


// general
extern GAUGEHDR gaugehdr_multicrewgauge;
extern HMODULE gInstance;
extern SmartPtr<MulticrewCore> core;

// classes
class Element;
class IconElement;
class Gauge;
class MulticrewGauge;

class Element {
public:
	Element( int id, Gauge &gauge, ELEMENT_HEADER *elementHeader );
	virtual ~Element();

	ELEMENT_HEADER *elementHeader();
	Gauge &gauge();
	int id();

	virtual void sendProc() {};
	virtual void receive( void *data, unsigned size ) {}

private:
	struct Data;
	Data *d;
};


class IconElement : public Element {
public:
	IconElement( int id, Gauge &gauge, ELEMENT_ICON *iconHeader );
	virtual ~IconElement();

	ELEMENT_ICON *iconHeader();
	
protected:
	virtual FLOAT64 callback( PELEMENT_ICON pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class IconRecorder : public IconElement {
public:
	IconRecorder( int id, Gauge &gauge, ELEMENT_ICON *iconHeader );
	virtual ~IconRecorder();

	virtual void sendProc();

private:
	virtual FLOAT64 callback( PELEMENT_ICON pelement );
};


class IconViewer : public IconElement {
public:
	IconViewer( int id, Gauge &gauge, ELEMENT_ICON *iconHeader );
	virtual ~IconViewer();

	virtual void receive( void *data, unsigned size );

private:	
	virtual FLOAT64 callback( PELEMENT_ICON pelement );
};


class NeedleElement : public Element {
public:
	NeedleElement( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader );
	virtual ~NeedleElement();

	ELEMENT_NEEDLE *needleHeader();
	
protected:
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class NeedleRecorder : public NeedleElement {
public:
	NeedleRecorder( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader );
	virtual ~NeedleRecorder();

	virtual void sendProc();

private:	
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );
};


class NeedleViewer : public NeedleElement {
public:
	NeedleViewer( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader );
	virtual ~NeedleViewer();

	virtual void receive( void *data, unsigned size );

private:	
	virtual FLOAT64 callback( PELEMENT_NEEDLE pelement );
};


class StringElement : public Element {
public:
	StringElement( int id, Gauge &gauge, ELEMENT_STRING *stringHeader );
	virtual ~StringElement();

	ELEMENT_STRING *stringHeader();
	
protected:
	virtual FLOAT64 callback( PELEMENT_STRING pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class StringRecorder : public StringElement {
public:
	StringRecorder( int id, Gauge &gauge, ELEMENT_STRING *stringHeader );
	virtual ~StringRecorder();

	virtual void sendProc();

private:	
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
};


class StringViewer : public StringElement {
public:
	StringViewer( int id, Gauge &gauge, ELEMENT_STRING *stringHeader );
	virtual ~StringViewer();

	virtual void receive( void *data, unsigned size );

private:	
	virtual FLOAT64 callback( PELEMENT_STRING pelement );
};


class StaticElement : public Element {
public:
	StaticElement( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader );
	virtual ~StaticElement();

	ELEMENT_STATIC_IMAGE *staticHeader();

protected:	
	struct Data;
	Data *d;
};


class StaticRecorder : public StaticElement {
public:
	StaticRecorder( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader );
	virtual ~StaticRecorder();
};


class StaticViewer : public StaticElement {
public:
	StaticViewer( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader );
	virtual ~StaticViewer();
};


class Gauge {
public:
	Gauge( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader );
	virtual ~Gauge();

	const std::deque<Element*> &elements();
	PGAUGEHDR gaugeHeader() ;
	std::string name();
	std::string parameter();
	int id();
	MulticrewGauge *mgauge();
	virtual void sendProc();

	void send( int element, void *data, unsigned size, bool safe );
	virtual void receive( int element, void *data, unsigned size );

	static Gauge &gauge( const std::string &name, const std::string &parameter );

 protected:	
	void createElements();
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data )=0;
	virtual Element *createElement( int id, PELEMENT_HEADER pelement )=0;

	struct Data;
	friend Data;
	Data *d;
};


class GaugeRecorder : public Gauge {
public:
	GaugeRecorder( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader );
	virtual ~GaugeRecorder();

private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
};


class GaugeViewer : public Gauge  {
public:
	GaugeViewer( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader );
	virtual ~GaugeViewer();

private:
	virtual void callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual Element *createElement( int id, PELEMENT_HEADER pelement );
};

class MulticrewGauge : public MulticrewModule {
 public:
	MulticrewGauge( bool hostMode, std::string clientName );
	virtual ~MulticrewGauge();
	bool init();

	void send( int gauge, int element, void *data, unsigned size, bool safe );
	PGAUGE_CALLBACK installCallback();

 protected:	
	virtual void receive( void *data, unsigned size );
	void installGauge( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data );
	virtual void sendProc();
	
 private:
	struct Data;
	friend Data;
	Data *d;
};
