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

using namespace Gdiplus;


enum PacketType {
	elementPacket=0,
	mousePacket,
};

#pragma pack(push,1)
struct BasePacket {	
	PacketType type;
};

struct ElementPacket : public BasePacket {
	int element;
	char data[0];
};

struct MousePacket : public BasePacket {
	int mouseRectNum;
	PIXPOINT pix;
	FLAGS32 flags;
};
#pragma pack(pop,1)



static std::map<std::string, Gauge*> namedGauges;

struct Gauge::Data {
	Data( Gauge *gauge )
		: callbackAdapter( gauge, Gauge::callback ) {
	}

	PGAUGEHDR gaugeHeader;
	PGAUGE_CALLBACK originalGaugeCallback;
	VoidCallbackAdapter3<Gauge, PGAUGEHDR, SINT32, UINT32> callbackAdapter;

	// mouse data
	std::deque<PMOUSE_FUNCTION> originalMouseCallbacks;
	typedef UserCallbackAdapter2<BOOL, Gauge, int, PPIXPOINT, FLAGS32> MouseCallback;
	std::deque<MouseCallback*> mouseCallbackAdapters;

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


Gauge::Gauge( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader ) {
	d = new Data( this );
	d->mgauge = mgauge;
	d->gaugeHeader = gaugeHeader;
	d->name = d->gaugeHeader->gauge_name;	
	d->parameters = (d->gaugeHeader->parameters!=NULL)?d->gaugeHeader->parameters:"";
	d->id = id;
	namedGauges[name() + "§" + parameter()] = this;
	InitializeCriticalSection( &d->cs );
	
	// setup callback
	d->originalGaugeCallback = d->gaugeHeader->gauge_callback;
	if( d->originalGaugeCallback!=NULL )		
		d->gaugeHeader->gauge_callback = d->callbackAdapter.callback();	

	// setup mouse callbacks
	if( d->gaugeHeader->mouse_rect!=0 ) {
		PMOUSERECT rect = d->gaugeHeader->mouse_rect;
		int num = 0;
	   	while( rect->rect_type!=MOUSE_RECT_EOL ) {
			d->originalMouseCallbacks.push_back( rect->mouse_function );
			//if( rect->mouse_function!=0 ) {
				dout << "Wrapping mouse callback for " << d->name << std::endl;
				Data::MouseCallback *callback = new Data::MouseCallback( this, Gauge::mouseCallback, num );
				d->mouseCallbackAdapters.push_back( callback );
				//}
				rect->mouse_function = callback->callback();

			num++;
			rect++;
		}
	}
}

void Gauge::createElements() {
	// create elements
	if( d->gaugeHeader->elements_list[0]!=NULL ) {
		std::stack<ELEMENT_HEADER*> todo;
		todo.push( d->gaugeHeader->elements_list[0] );
		int id = 0;
		
		while( !todo.empty() ) {
			ELEMENT_HEADER *pelement = todo.top(); todo.pop();
			Element *element = createElement( id, pelement );
			if( element!=0 ) {
				EnterCriticalSection( &d->cs );
				d->elements.push_back( element );			
				LeaveCriticalSection( &d->cs );
				id++;
			}

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

	// delete all elements
	std::deque<Element*>::iterator it = d->elements.begin();
	while( it!=d->elements.end() ) delete *it++;
	d->elements.clear();

	// delete mouse callbacks
	std::deque<Data::MouseCallback*>::iterator it2 = d->mouseCallbackAdapters.begin();
	while( it2!=d->mouseCallbackAdapters.end() ) delete *it2++;
	d->mouseCallbackAdapters.clear();

	// delete from global map
	namedGauges.erase( name() + "§" + parameter() );

	LeaveCriticalSection( &d->cs );
	DeleteCriticalSection( &d->cs );
	delete d;
}

PGAUGEHDR Gauge::gaugeHeader() { 
	return d->gaugeHeader; 
}

const std::deque<Element*> &Gauge::elements() {
	return d->elements;
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


Gauge &Gauge::gauge( const std::string &name, const std::string &parameter ) {	
	return *namedGauges[name + "§" + parameter];
}


MulticrewGauge *Gauge::mgauge() {
	return d->mgauge;
}


void Gauge::send( int element, void *data, unsigned size, bool safe, bool append ) {
	if( !append ) {
		// add route
		ElementPacket p;
		p.type = elementPacket;
		p.element = element;
		d->mgauge->send( id(), &p, sizeof(ElementPacket), safe );
	}
	
	// add data
	d->mgauge->send( id(), data, size, safe, true );
}


void Gauge::receive( void *data, unsigned size ) {
	BasePacket *p = (BasePacket*)data;
	switch( p->type ) {
	case elementPacket: 
	{
		ElementPacket *ep = (ElementPacket*)p;
		EnterCriticalSection( &d->cs );
		if( ep->element>=0 && ep->element<d->elements.size() ) {
			Element *element = d->elements[ep->element];
			LeaveCriticalSection( &d->cs );
			element->receive( ep->data, size-sizeof(ElementPacket) );
		} else {
			LeaveCriticalSection( &d->cs );
		}
	}
	break;
	default:
		break;
	}
}


void Gauge::sendProc() {
	EnterCriticalSection( &d->cs );
	for( int i=0; i<d->elements.size(); i++ )
		d->elements[i]->sendProc();
	LeaveCriticalSection( &d->cs );
}


/********************************************************************************************/

GaugeRecorder::GaugeRecorder( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader ) 
	: Gauge( mgauge, id, gaugeHeader ) {
	createElements();
}

GaugeRecorder::~GaugeRecorder() {
}

Element *GaugeRecorder::createElement( int id, PELEMENT_HEADER pelement ) {
	switch( pelement->element_type ) {
		case ELEMENT_TYPE_ICON: return new IconRecorder( id, *this, (ELEMENT_ICON*)pelement );					
		case ELEMENT_TYPE_NEEDLE: return new NeedleRecorder( id, *this, (ELEMENT_NEEDLE*)pelement );					
		case ELEMENT_TYPE_STRING: return new StringRecorder( id, *this, (ELEMENT_STRING*)pelement );					
		case ELEMENT_TYPE_STATIC_IMAGE: return new StaticRecorder( id, *this, (ELEMENT_STATIC_IMAGE*)pelement );			
		default: return 0;			
	}
}

void GaugeRecorder::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	//dout << service_id << std::endl;
	//dout << "> Gauge::callback " << this << " service_id=" << service_id << std::endl;
	bool ispfd = false;
	if( name()=="EFISDisplay" && parameter()=="PFD 1" ) ispfd=true;

	// do work of callback
	switch (service_id) {
    	case PANEL_SERVICE_POST_INSTALL:
			if( ispfd ) GdiplusStartup(&d->gdiplusToken, &d->gdiplusStartupInput, NULL);
			if( d->originalGaugeCallback!=NULL ) (*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );
			break;

		case PANEL_SERVICE_PRE_DRAW:
			if( ispfd ) {
				Element *element = elements()[0];
				PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();

				// create metafile
				Metafile metafile( staticImage->hdc, EmfTypeEmfPlusOnly );
				
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
			} else
				if( d->originalGaugeCallback!=NULL ) (*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );				
			break;

		case PANEL_SERVICE_DISCONNECT:
			if( ispfd ) GdiplusShutdown(d->gdiplusToken);
			break;

		case PANEL_SERVICE_PRE_KILL:			
			d->gaugeHeader->gauge_callback = d->originalGaugeCallback;
			if( d->originalGaugeCallback!=NULL ) (*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );			
			break;

		default:
			if( d->originalGaugeCallback!=NULL ) (*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );			
			break;
	}

	// draw debug info
	if( ispfd && service_id==PANEL_SERVICE_PRE_DRAW) {		
		PELEMENT_STATIC_IMAGE pelement = (PELEMENT_STATIC_IMAGE)(d->gaugeHeader->elements_list[0]);

		if (pelement && pelement->hdc!=NULL) {
			//OutputDebugString("pelement\n");

			HDC hdc = pelement->hdc;
			PIXPOINT dim = pelement->image_data.final->dim;
			if (hdc) {
				// clear background
				SelectObject (hdc, GetStockObject (NULL_BRUSH));
				SelectObject (hdc, GetStockObject (WHITE_PEN));
				Rectangle (hdc, 0, 0, dim.x, dim.y);

				/*char out[256];
				sprintf( out, "hdc %i\n", (int)hdc );
				OutputDebugString(out);*/
				static int col = 1234;
				Graphics graphics(hdc);
				Pen      pen(Color(255, 255, 255, 255), 10.0);
				graphics.DrawLine(&pen, dim.x/10.0f, dim.x/10.0f, dim.x/10.0f*9.0f, dim.y/10.0f*9.0f);
				col = col*13+1347;
			}

			SET_OFF_SCREEN (pelement);
		}
	}
	//dout << "< Gauge::callback" << std::endl;
}


void GaugeRecorder::receive( void *data, unsigned size ) {
	BasePacket *p = (BasePacket*)data;
	switch( p->type ) {
	case mousePacket:
	{
		// emit mouse event
		MousePacket *mp = (MousePacket*)data;
		EnterCriticalSection( &d->cs );

		dout << "mouse packet for " << d->name << " mouserect " << mp->mouseRectNum << std::endl;

		if( mp->mouseRectNum>=0 && mp->mouseRectNum<d->originalMouseCallbacks.size() ) {
			PMOUSE_FUNCTION cb = d->originalMouseCallbacks[mp->mouseRectNum];
			PMOUSERECT rect = d->gaugeHeader->mouse_rect + mp->mouseRectNum;

			// has callback?
			if( cb!=0 ) {
				// call callback
				dout << "Call mouse callback for " << d->name << std::endl;
				cb( &mp->pix, mp->flags );
			} else {
				// has eventid?
				if( rect->event_id!=0 ) {
					// trigger event
					dout << "Trigger key event " << rect->event_id << " from " << d->name << std::endl;
					trigger_key_event( rect->event_id, 0 );
				}
			}			 
		} 
		LeaveCriticalSection( &d->cs );
	}
	break;
	default:
		Gauge::receive( data, size );
		break;
	}
}


BOOL GaugeRecorder::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	if( mouseRectNum>=0 && mouseRectNum<d->originalMouseCallbacks.size() && d->originalMouseCallbacks[mouseRectNum]!=0 ) {
		dout << "mouseCallback for " << d->name << std::endl;
		return d->originalMouseCallbacks[mouseRectNum]( pix, flags );
	}
	
	return TRUE;
}


/************************************************************************************/

GaugeViewer::GaugeViewer( MulticrewGauge *mgauge, int id, PGAUGEHDR gaugeHeader ) 
	: Gauge( mgauge, id, gaugeHeader ) {
	createElements();
}

GaugeViewer::~GaugeViewer() {
}

void GaugeViewer::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call original callback
	if( d->originalGaugeCallback!=NULL ) 
		(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );
}

Element *GaugeViewer::createElement( int id, PELEMENT_HEADER pelement ) {
	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: return new IconViewer( id, *this, (ELEMENT_ICON*)pelement );					
	case ELEMENT_TYPE_NEEDLE: return new NeedleViewer( id, *this, (ELEMENT_NEEDLE*)pelement );					
	case ELEMENT_TYPE_STRING: return new StringViewer( id, *this, (ELEMENT_STRING*)pelement );					
	case ELEMENT_TYPE_STATIC_IMAGE: return new StaticViewer( id, *this, (ELEMENT_STATIC_IMAGE*)pelement );			
	default: return 0;			
	}
}

BOOL GaugeViewer::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	dout << "mouseCallback for " << d->name << std::endl;
	MousePacket p;
	p.type = mousePacket;
	memcpy( &p.pix, pix, sizeof(PIXPOINT) );
	p.flags = flags;
	p.mouseRectNum = mouseRectNum;

	d->mgauge->send( id(), &p, sizeof(MousePacket), true );
	
	return TRUE;
}
