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

#include "../multicrewcore/streams.h"
#include "multicrewgauge.h"
#include "../multicrewcore/callback.h"

#define BUFFER_SIZE 256

struct StringElement::Data {	
	Data( StringElement *el ) 
		: callbackAdapter( el, StringElement::callback ) {
	}
	ELEMENT_STRING *stringHeader;
	PSTRING_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, StringElement, PELEMENT_STRING> callbackAdapter;

	char value[BUFFER_SIZE+1];
	char buffer[BUFFER_SIZE+1];
	bool changed;
	CRITICAL_SECTION cs;
};

StringElement::StringElement( int id, Gauge &gauge )
	: Element( id, gauge ) {
	d = new Data( this );
	d->stringHeader = 0;
	d->value[0] = 0;
	d->value[BUFFER_SIZE] = 0;
	d->buffer[0] = 0;
	d->buffer[BUFFER_SIZE] = 0;
	d->changed = true;
	d->origCallback = 0;

	InitializeCriticalSection( &d->cs );
}

StringElement::~StringElement() {
	detach();
	DeleteCriticalSection( &d->cs );
	delete d;
}

ELEMENT_STRING *StringElement::stringHeader() { 
	return d->stringHeader; 
}

void StringElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->stringHeader = (ELEMENT_STRING*)elementHeader;
	d->origCallback = d->stringHeader->update_cb;

	if( d->stringHeader->update_cb!=NULL ) {
		// install callback wrapper	
		dout << "install updateCallback for " << d->stringHeader << std::endl;
		d->stringHeader->update_cb = d->callbackAdapter.callback();
	}
}

void StringElement::detach() {
	Element::detach();
	if( d->stringHeader ) {
		d->stringHeader->update_cb = d->origCallback;
		d->stringHeader->string = strdup(""); // minimal memory leak to avoid segfaults
	}
}

/****************************************************************************************/

StringRecorder::StringRecorder( int id, Gauge &gauge )
	: StringElement( id, gauge ) {
}

StringRecorder::~StringRecorder() {
}

FLOAT64 StringRecorder::callback( PELEMENT_STRING pelement ) {	
	//dout << "> callback " << d->stringHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	EnterCriticalSection( &d->cs );
	if( (pelement->string==0 && d->value[0]!=0) 
		|| strcmp(pelement->string, d->value)!=0 ) {
		dout << "String callback " << d->stringHeader << ":" << this 
			 << " in " << gauge().name() << " = " << (unsigned long)ret << std::endl;
		
		// empty string?
		if( pelement->string==0 )
			d->value[0] = 0;
		else
			strncpy( d->value, pelement->string, BUFFER_SIZE );
		
		d->changed = true;
	}
	//dout << "< cal lback " << d->stringHeader << std::endl;
	LeaveCriticalSection( &d->cs );
	return ret;
}


void StringRecorder::sendProc() {
	if( d->changed ) {
		EnterCriticalSection( &d->cs );
		gauge().send( id(), d->value, strlen(d->value)+1, false );
		d->changed = false;
		LeaveCriticalSection( &d->cs );
	}
}


/****************************************************************************************/

StringViewer::StringViewer( int id, Gauge &gauge )
	: StringElement( id, gauge ) {
}

StringViewer::~StringViewer() {
}

FLOAT64 StringViewer::callback( PELEMENT_STRING pelement ) {
	EnterCriticalSection( &d->cs );
	FLOAT64 ret = (*d->origCallback)( pelement );
	pelement->string = (char*)0x4d434d43;
	pelement->string = d->buffer;
	strncpy( d->buffer, d->value, BUFFER_SIZE );
	LeaveCriticalSection( &d->cs );
	return ret;
}


void StringViewer::receive( void *data, unsigned size ) {
	EnterCriticalSection( &d->cs );
	strncpy( d->value, (char*)data, BUFFER_SIZE );
	LeaveCriticalSection( &d->cs );
}
