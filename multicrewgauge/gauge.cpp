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
#include <Gdiplus.h>
#include <stack>
#include <map>
#include <set>

#include "gauges.h"
#include "multicrewgauge.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/callback.h"
#include "zdelta/zdlib.h"
#include "zdelta/zd_mem.h"

using namespace Gdiplus;



class GlobalMem : public Shared {
public:
	GlobalMem( HGLOBAL hmem ) {
		this->hmem = hmem;
	}

	GlobalMem( unsigned size ) {
		this->hmem = GlobalAlloc( 0, size );
	}

	GlobalMem( void *data, unsigned size ) {
		this->hmem = GlobalAlloc( 0, size );
		memcpy( lock(), data, size );
		unlock();
	}

	virtual ~GlobalMem() {
		GlobalFree( hmem );
	}

	void *lock() {
		return GlobalLock( hmem );
	}

	void unlock() {
		GlobalUnlock( hmem );
	}

	unsigned size() {
		return GlobalSize( hmem );
	}

	HGLOBAL handle() {
		return hmem;
	}

private:
	HGLOBAL hmem;
};


enum {
	mousePacket = 0,
	elementPacket,
	metafilePacket
};


struct MouseStruct {
	int mouseRectNum;
	PIXPOINT pix;
	FLAGS32 flags;
};


typedef TypedPacket<char, Packet> GaugePacket;
typedef TypedPacket<unsigned, Packet> ElementPacket;
typedef StructPacket<MouseStruct> MousePacket;

class MetafilePacket : public WrappedPacket<PIXPOINT,RawPacket> {
public:
	MetafilePacket( PPIXPOINT dim, SharedBuffer buffer ) 
		: WrappedPacket<PIXPOINT,RawPacket>(
			*dim,
			new RawPacket(buffer) ) {
	}

	MetafilePacket( SharedBuffer &buf ) 
		: WrappedPacket<PIXPOINT,RawPacket>(buf) {
	}

	PPIXPOINT dimension() {
		return prefix();
	}

	SharedBuffer buffer() {
		return wrappee()->buffer();
	}

protected:
	virtual SmartPtr<RawPacket> createWrappee( SharedBuffer &buffer ) {
		return new RawPacket( buffer );
	}
};


class ElementPacketFactory : public TypedPacketFactory<unsigned,Packet> {
public:
	ElementPacketFactory( std::deque<Element *> &elements ) 
		: _elements( elements ) {
	}

	virtual SmartPtr<Packet> createPacket( unsigned key, SharedBuffer &buffer ) {
		if( key<_elements.size() )
			return _elements[key]->createPacket( buffer );
		else
			return 0;
	}

private:
	std::deque<Element *> &_elements;
};


class GaugePacketFactory : public TypedPacketFactory<char,Packet> {
public:
	GaugePacketFactory( ElementPacketFactory& elementFactory ) 
		: _elementFactory( elementFactory ) {
	}

	virtual SmartPtr<Packet> createPacket( char key, SharedBuffer &buffer ) {
		switch( key ) {
		case mousePacket:
			return new MousePacket( buffer );
		case elementPacket:		
			return new ElementPacket( buffer, &_elementFactory );
		case metafilePacket:
			return new MetafilePacket( buffer );
		default:
			return 0;
		}
	}

private:
	ElementPacketFactory& _elementFactory;
};



struct Gauge::Data {
	Data( Gauge *gauge )
		: callbackAdapter( gauge, Gauge::callback ),
		elementPacketFactory( elements ),
		packetFactory( elementPacketFactory ) {
	}

	PGAUGEHDR gaugeHeader;
	PGAUGE_CALLBACK originalGaugeCallback;
	VoidCallbackAdapter3<Gauge, PGAUGEHDR, SINT32, UINT32> callbackAdapter;
	ElementPacketFactory elementPacketFactory;
	GaugePacketFactory packetFactory;

	// mouse data
	std::deque<PMOUSE_FUNCTION> originalMouseCallbacks;
	typedef UserCallbackAdapter2<BOOL, Gauge, int, PPIXPOINT, FLAGS32> MouseCallback;
	std::deque<MouseCallback*> mouseCallbackAdapters;
	typedef SmartPtr<MousePacket> SmartMousePacket;
	std::deque<SmartMousePacket> mouseEvents;
	PMOUSERECT origMouseRect;

	// metafile data
	SmartPtr<GlobalMem> lastMetafile;
	PIXPOINT metafileDim;

	// general gauge data
	std::deque<Element *> elements;	
	std::string name;
	std::string parameters;
	int id;
	MulticrewGauge *mgauge;
	CRITICAL_SECTION cs;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;	
};


Gauge::Gauge( MulticrewGauge *mgauge, int id ) {
	d = new Data( this );
	d->mgauge = mgauge;
	d->gaugeHeader = 0;
	d->name = "";
	d->parameters = "";
	d->id = id;

	InitializeCriticalSection( &d->cs );
}

void Gauge::createElements() {
	// create elements
	if( d->gaugeHeader->elements_list[0]!=NULL ) {
		std::stack<ELEMENT_HEADER*> todo;
		todo.push( d->gaugeHeader->elements_list[0] );
		int id = 0;
		
		while( !todo.empty() ) {
			ELEMENT_HEADER *pelement = todo.top(); todo.pop();

			EnterCriticalSection( &d->cs );

			// element not yet created?
			if( id>=d->elements.size() ) 
				d->elements.push_back( createElement( id, pelement ) );

			// attach element
			if( d->elements[id] ) d->elements[id]->attach( pelement );

			id++;
			LeaveCriticalSection( &d->cs );

			if( pelement->next_element!=NULL ) {
				for( int i=0; pelement->next_element[i]!=NULL; i++ )
					todo.push( pelement->next_element[i] );				
			}
		}
	}
}

Gauge::~Gauge() {
	dout << "~Gauge " << name() << " " << parameter() << std::endl;			
	EnterCriticalSection( &d->cs );

	detach();

	// delete all elements
	std::deque<Element*>::iterator it = d->elements.begin();
	while( it!=d->elements.end() ) delete *it++;
	d->elements.clear();

	LeaveCriticalSection( &d->cs );
	DeleteCriticalSection( &d->cs );
	delete d;
}

PGAUGEHDR Gauge::gaugeHeader() { 
	return d->gaugeHeader; 
}

std::string Gauge::name() {	
	return d->name;
}

std::string Gauge::parameter() {	
	return d->parameters;
}

int Gauge::id() {
	return d->id;
}


MulticrewGauge *Gauge::mgauge() {
	return d->mgauge;
}


void Gauge::send( unsigned element, SmartPtr<Packet> packet, bool safe ) {
	d->mgauge->send( id(), 
					 new GaugePacket( 
						 elementPacket,
						 new ElementPacket(element, packet)),
					 safe );
}


void Gauge::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case elementPacket: 
	{
		SmartPtr<ElementPacket> ep = (ElementPacket*)&*gp->wrappee();
		EnterCriticalSection( &d->cs );
		if( ep->key()>=0 && ep->key()<d->elements.size() ) {
			Element *element = d->elements[ep->key()];
			LeaveCriticalSection( &d->cs );
			if( element ) element->receive( ep->wrappee() );
		} else
			LeaveCriticalSection( &d->cs );
	} break;
	case mousePacket:
		EnterCriticalSection( &d->cs );
		d->mouseEvents.push_back( (MousePacket*)&*gp->wrappee() );
		LeaveCriticalSection( &d->cs );
		break;
	default:
		break;
	}
}


SmartPtr<Packet> Gauge::createPacket( SharedBuffer &buffer ) {
	EnterCriticalSection( &d->cs );
	SmartPtr<GaugePacket> gp = new GaugePacket( buffer, &d->packetFactory );
	LeaveCriticalSection( &d->cs );
	return gp;
}


void Gauge::sendProc() {
    // call element's sendProc
	EnterCriticalSection( &d->cs );
	for( int i=0; i<d->elements.size(); i++ ) {
		Element *element = d->elements[i];
		if( element ) element->sendProc();
	}
	LeaveCriticalSection( &d->cs );
}


void Gauge::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call original callback
	if( d->originalGaugeCallback!=NULL ) 
		(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );

	if( service_id>=12 ) {
		dout << d->name << " " << service_id << std::endl;
	}

	// if a pre kill is received the plane is either closed
	// or the panel is resized. In the latter case all gauges will be
	// recreated during the next moments
	if( service_id==PANEL_SERVICE_PRE_KILL ) {
		//dout << "PANEL_SERVICE_PRE_KILL" << std::endl;
		detach();
	}
}

void Gauge::attach( PGAUGEHDR gaugeHeader ) {	
	EnterCriticalSection( &d->cs );

	// detach first if attached
	if( d->gaugeHeader ) detach();

	// initialize general variables
	d->gaugeHeader = gaugeHeader;
	d->name = gaugeHeader->gauge_name;	
	d->parameters = (gaugeHeader->parameters!=NULL)?gaugeHeader->parameters:"";

	bool hookElements = configBoolValue( "hookelements", true );
	if( hookElements ) createElements();
	
	// setup callback
	d->originalGaugeCallback = d->gaugeHeader->gauge_callback;
	if( d->originalGaugeCallback!=NULL )		
		d->gaugeHeader->gauge_callback = d->callbackAdapter.callback();	

	// setup mouse callbacks
	bool hookMouse = configBoolValue( "hookmouse", true );
	d->origMouseRect = d->gaugeHeader->mouse_rect;
	if( d->gaugeHeader->mouse_rect!=0 && hookMouse ) {
		PMOUSERECT rect = d->gaugeHeader->mouse_rect;
		int num = 0;
	   	while( rect->rect_type!=MOUSE_RECT_EOL ) {
			d->originalMouseCallbacks.push_back( rect->mouse_function );
			if( rect->mouse_function!=0 || rect->event_id!=0 ) {
				//dout << "Wrapping mouse callback for " << d->name << std::endl;
				Data::MouseCallback *callback = new Data::MouseCallback( this, Gauge::mouseCallback, num );
				d->mouseCallbackAdapters.push_back( callback );
				rect->mouse_function = callback->callback();
			}

			num++;
			rect++;
		}
	}

	dout << "gaugeHeader=" << gaugeHeader << std::endl;

	LeaveCriticalSection( &d->cs );
}

bool Gauge::configBoolValue( const std::string &key, bool def ) {
	// first look in [name!parameter], then in [name], then def
	Config *c = d->mgauge->config();
	return c->boolValue( name()+"!"+parameter(), key,
						 c->boolValue( name(), key, def ) );
}

void Gauge::detach() {
	if( d->gaugeHeader ) {
		EnterCriticalSection( &d->cs );

		// restore gauge callback
		d->gaugeHeader->gauge_callback = d->originalGaugeCallback;

		// restore mouse callbacks
		if( d->gaugeHeader->mouse_rect!=0 && d->origMouseRect==d->gaugeHeader->mouse_rect ) {
			PMOUSERECT rect = d->gaugeHeader->mouse_rect;
			int num = 0;
			while( rect->rect_type!=MOUSE_RECT_EOL ) {
				rect->mouse_function = d->originalMouseCallbacks[num];
				num++;
				rect++;
			}
		}	

		// detach all elements
		std::deque<Element*>::iterator it = d->elements.begin();
		while( it!=d->elements.end() ) {
			Element *element = *(it++);
			if( element ) element->detach();
		}
										   
		// delete mouse callbacks
		std::deque<Data::MouseCallback*>::iterator it2 = d->mouseCallbackAdapters.begin();
		while( it2!=d->mouseCallbackAdapters.end() ) delete *it2++;
		d->mouseCallbackAdapters.clear();
		d->originalMouseCallbacks.clear();

		d->mgauge->detached( this );
		d->gaugeHeader = 0;
		LeaveCriticalSection( &d->cs );
	}
}


/********************************************************************************************/

GaugeRecorder::GaugeRecorder( MulticrewGauge *mgauge, int id ) 
	: Gauge( mgauge, id ) {
}

GaugeRecorder::~GaugeRecorder() {
}

Element *GaugeRecorder::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;
	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: el = new IconRecorder( id, *this ); break;
	case ELEMENT_TYPE_NEEDLE: el = new NeedleRecorder( id, *this ); break;
	case ELEMENT_TYPE_STRING: el = new StringRecorder( id, *this ); break;
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticRecorder( id, *this ); break;
	default: break;
	}

	return el;
}

void GaugeRecorder::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_PRE_DRAW: {
		// handle mouse events which came in from the network
		EnterCriticalSection( &d->cs );
		for( std::deque<Data::SmartMousePacket>::iterator it = d->mouseEvents.begin();
			 it!=d->mouseEvents.end();
			 it++ ) {
			// emit mouse event
			dout << "mouse packet for " << d->name << " mouserect " << (*it)->data().mouseRectNum << std::endl;
			
			if( (*it)->data().mouseRectNum>=0 && (*it)->data().mouseRectNum<d->originalMouseCallbacks.size() ) {
				PMOUSE_FUNCTION cb = d->originalMouseCallbacks[(*it)->data().mouseRectNum];
				PMOUSERECT rect = d->gaugeHeader->mouse_rect + (*it)->data().mouseRectNum;
				
				// has callback?
				if( cb!=0 ) {
					// call callback
					dout << "Call mouse callback for " << d->name << std::endl;
					cb( &(*it)->data().pix, (*it)->data().flags );
				} else {
					// has eventid?
					if( rect->event_id!=0 ) {
						// trigger event
						dout << "Trigger key event " << rect->event_id << " from " << d->name << std::endl;
						trigger_key_event( rect->event_id, 0 );
					}
				}
			}
		}
		d->mouseEvents.clear();
		LeaveCriticalSection( &d->cs );
	} break;

		
	default:
		break;
	}
	
	// call inherited callback
	Gauge::callback( pgauge, service_id, extra_data );
}


void GaugeRecorder::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case mousePacket:
		EnterCriticalSection( &d->cs );
		dout << "Received mouse packet for " << d->name << std::endl;
		d->mouseEvents.push_back( (MousePacket*)&*gp->wrappee() );
		LeaveCriticalSection( &d->cs );
		break;

	default:
		Gauge::receive( packet );
		break;
	}
}


BOOL GaugeRecorder::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	EnterCriticalSection( &d->cs );
	if( mouseRectNum>=0 && mouseRectNum<d->originalMouseCallbacks.size() && d->originalMouseCallbacks[mouseRectNum]!=0 ) {
		dout << "mouseCallback for " << d->name << std::endl;
		PMOUSE_FUNCTION cb = d->originalMouseCallbacks[mouseRectNum];
		cb( pix, flags );
	}
	LeaveCriticalSection( &d->cs );
	
	return TRUE;
}

/**************************************************************************/

GaugeMetafileRecorder::GaugeMetafileRecorder( MulticrewGauge *mgauge, int id ) 
	: GaugeRecorder( mgauge, id ) {
}

GaugeMetafileRecorder::~GaugeMetafileRecorder() {
}

Element *GaugeMetafileRecorder::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;
	switch( pelement->element_type ) {
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticRecorder( id, *this ); break;
	default: break;
	}

	return el;
}

void GaugeMetafileRecorder::sendProc() {
	Gauge::sendProc();

	// send metafile
	EnterCriticalSection( &d->cs );	
	if( !d->lastMetafile.isNull() ) {
		d->mgauge->send( 
			id(), 
			new GaugePacket( 
				metafilePacket,
				new MetafilePacket( 
					&d->metafileDim,
					SharedBuffer( d->lastMetafile->lock(), 
								  d->lastMetafile->size() ))),
			false );
		d->lastMetafile->unlock();
		d->lastMetafile = 0;
	}
	LeaveCriticalSection( &d->cs );
}

void GaugeMetafileRecorder::callback( PGAUGEHDR pgauge, SINT32 service_id, 
									  UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_POST_INSTALL:
		GdiplusStartup(&d->gdiplusToken, &d->gdiplusStartupInput, NULL);
		break;
		
	case PANEL_SERVICE_PRE_DRAW: {
		// record metafile for gauge output
		EnterCriticalSection( &d->cs );
		if( d->elements.size()>0 && d->elements[0] ) {
			
			Element *element = d->elements[0];
			PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();
		
			// create memory stream
			IStream *stream;
			CreateStreamOnHGlobal( NULL, FALSE, &stream );
		 	
			// create metafile
			Metafile metafile( stream, staticImage->hdc, EmfTypeEmfPlusOnly );
			
			// let gauge draw to metafile
			HDC origHdc = staticImage->hdc;
			Graphics *metaGraphics = new Graphics( &metafile );
			metaGraphics->SetSmoothingMode( SmoothingModeAntiAlias );
			staticImage->hdc = metaGraphics->GetHDC();				
			(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );
			metaGraphics->ReleaseHDC( staticImage->hdc );
			delete metaGraphics;
			staticImage->hdc = origHdc;
			origHdc = (HDC)0x204c5047;
			
			// display metafile
			Graphics graphics( staticImage->hdc );
			graphics.SetSmoothingMode( SmoothingModeAntiAlias );
			graphics.DrawImage( &metafile, 0, 0 );
			
			// save metafile for later sending
			HGLOBAL hmem;
			GetHGlobalFromStream( stream, &hmem );
			stream->Release();
			if( GlobalSize(hmem)>500 ) {
				EnterCriticalSection( &d->cs );
				d->lastMetafile = new GlobalMem(hmem);
				memcpy( &d->metafileDim, 
						&staticImage->image_data.final->dim, 
						sizeof(PIXPOINT) );
				LeaveCriticalSection( &d->cs );
			} else
				GlobalFree( hmem );
				 
			LeaveCriticalSection( &d->cs );
			return;
		}				
		LeaveCriticalSection( &d->cs );
	} break;

	case PANEL_SERVICE_DISCONNECT:
		GdiplusShutdown(d->gdiplusToken);
		break;
		
	default:
		break;
	}
	
	// call inherited callback
	Gauge::callback( pgauge, service_id, extra_data );
}


/**************************************************************************/

GaugeViewer::GaugeViewer( MulticrewGauge *mgauge, int id ) 
	: Gauge( mgauge, id ) {
}

GaugeViewer::~GaugeViewer() {
}


Element *GaugeViewer::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;

	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: el = new IconViewer( id, *this ); break;	
	case ELEMENT_TYPE_NEEDLE: el = new NeedleViewer( id, *this ); break;
	case ELEMENT_TYPE_STRING: el = new StringViewer( id, *this ); break;
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticViewer( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeViewer::sendProc() {
	Gauge::sendProc();

	// send mouse events
	EnterCriticalSection( &d->cs );
	std::deque<Data::SmartMousePacket>::iterator it = d->mouseEvents.begin();
	while( it!=d->mouseEvents.end() ) {
		d->mgauge->send( 
			id(), 
			new GaugePacket( 
				mousePacket,
				&*(*it)), 
			false );		
		it++;
	}
	d->mouseEvents.clear();
	LeaveCriticalSection( &d->cs );
}


BOOL GaugeViewer::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	dout << "mouseCallback for " << d->name << std::endl;

	// append mouse event to d->mouseEvents which is sent during the 
    // next sendProc call
	EnterCriticalSection( &d->cs );
	while( d->mouseEvents.size()>5 ) {
		// don't let the queue get too full
		d->mouseEvents.pop_front();
	}
	MouseStruct s;
	s.mouseRectNum = mouseRectNum;
	memcpy( &s.pix, pix, sizeof(PIXPOINT) );
	s.flags = flags;
	d->mouseEvents.push_back( new MousePacket( s ) );
	LeaveCriticalSection( &d->cs );
	
	return TRUE;
}


/**************************************************************************/

GaugeMetafileViewer::GaugeMetafileViewer( MulticrewGauge *mgauge, int id ) 
	: GaugeViewer( mgauge, id ) {
}

GaugeMetafileViewer::~GaugeMetafileViewer() {
}

void GaugeMetafileViewer::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_POST_INSTALL:
		GdiplusStartup(&d->gdiplusToken, &d->gdiplusStartupInput, NULL);
		break;
		
	case PANEL_SERVICE_PRE_DRAW: {
		// display
		EnterCriticalSection( &d->cs );
		if( d->elements.size()>0 && 
			d->elements[0] && 
			!d->lastMetafile.isNull() ) {
			dout << "draw metafile" << std::endl;

			Element *element = d->elements[0];
			PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();
			PPIXPOINT dim = &staticImage->image_data.final->dim;
			
			// create stream
			IStream *stream;
			CreateStreamOnHGlobal( d->lastMetafile->handle(), FALSE, &stream );			
			Metafile metafile( stream );
					
			// display metafile
			dout << "x=" << d->metafileDim.x << " -> x=" << dim->x << std::endl;
			Graphics graphics( staticImage->hdc );
			graphics.ScaleTransform( 
				((float)dim->x)/d->metafileDim.x, 
				((float)dim->y)/d->metafileDim.y, 
				MatrixOrderPrepend );
			graphics.SetSmoothingMode( SmoothingModeAntiAlias );
			graphics.DrawImage( &metafile, 0, 0 );
			
			stream->Release();
			d->lastMetafile = 0;
			SET_OFF_SCREEN( staticImage );
		}
		LeaveCriticalSection( &d->cs );
		return;
	} break;

	case PANEL_SERVICE_DISCONNECT:
		GdiplusShutdown(d->gdiplusToken);
		break;
		
	default:
		break;
	}

	GaugeViewer::callback( pgauge, service_id, extra_data );
}

Element *GaugeMetafileViewer::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;

	switch( pelement->element_type ) {
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticViewer( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeMetafileViewer::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case metafilePacket:
	{
		EnterCriticalSection( &d->cs );
		SmartPtr<MetafilePacket> mp = (MetafilePacket*)&*gp->wrappee();
		dout << "metafile packet with size " << mp->buffer().size() << std::endl;
		d->lastMetafile = new GlobalMem( mp->buffer().data(), 
										 mp->buffer().size() );
		memcpy( &d->metafileDim, mp->dimension(), sizeof(PIXPOINT) );
		LeaveCriticalSection( &d->cs );
	} break;

	default:
		Gauge::receive( packet );
		break;
	}
}
