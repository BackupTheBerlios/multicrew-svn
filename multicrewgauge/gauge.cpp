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
#include "../multicrewcore/debug.h"
#include "../multicrewcore/callback.h"

using namespace Gdiplus;

static std::map<std::string, Gauge*> namedGauges;

struct Gauge::Data {
	Data( Gauge *gauge )
		: callbackAdapter( gauge, Gauge::callback ) {
	}

	PGAUGEHDR gaugeHeader;
	PGAUGE_CALLBACK originalGaugeCallback;
	VoidCallbackAdapter3<Gauge, PGAUGEHDR, SINT32, UINT32> callbackAdapter;

	std::deque<Element *> elements;	
	std::string name;
	std::string parameters;
	int id;
	MulticrewGauge *mgauge;

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
	
	// setup callback
	d->originalGaugeCallback = d->gaugeHeader->gauge_callback;
	if( d->originalGaugeCallback!=NULL )		
		d->gaugeHeader->gauge_callback = d->callbackAdapter.callback();	
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
				d->elements.push_back( element );			
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

	// delete all elements
	std::deque<Element*>::iterator it = d->elements.begin();
	while( it!=d->elements.end() ) delete *it++;
	d->elements.clear();

	namedGauges.erase( name() + "§" + parameter() );

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


void Gauge::send( UpdatePacket *packet, bool safe ) {
	packet->gauge = id();
	d->mgauge->send( packet, safe );
}


void Gauge::receive( UpdatePacket *packet ) {
	if( packet->element<d->elements.size() && packet->element>=0 )
		d->elements[packet->element]->receive( packet );
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
