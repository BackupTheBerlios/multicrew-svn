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

#include "../multicrewcore/debug.h"
#include "multicrewgauge.h"
#include "../multicrewcore/callback.h"

struct StringElement::Data {	
	Data( StringElement *el ) 
		: callbackAdapter( el, StringElement::callback ) {
	}
	ELEMENT_STRING *stringHeader;
	PSTRING_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, StringElement, PELEMENT_STRING> callbackAdapter;

	char oldValue[32];
	char buffer[32];
};

StringElement::StringElement( int id, Gauge &gauge, ELEMENT_STRING *stringHeader )
	: Element( id, gauge, (ELEMENT_HEADER*)stringHeader ) {
	d = new Data( this );
	d->stringHeader = stringHeader;
	d->origCallback = stringHeader->update_cb;
	strcpy( d->oldValue, "" );

	// debug code for MasterCaution gauge
	if( /*gauge.name()=="MasterCaution" &&*/ d->stringHeader->update_cb!=NULL ) {
		// install callback wrapper	
		dout << "install updateCallback for " << d->stringHeader << std::endl;
		d->stringHeader->update_cb = d->callbackAdapter.callback();
	}
}

StringElement::~StringElement() {
	//d->stringHeader->update_cb = d->origUpdateCallback;	
	delete d;
}

ELEMENT_STRING *StringElement::stringHeader() { 
	return d->stringHeader; 
}

/****************************************************************************************/

StringRecorder::StringRecorder( int id, Gauge &gauge, ELEMENT_STRING *stringHeader )
	: StringElement( id, gauge, stringHeader ) {
}

StringRecorder::~StringRecorder() {
}

FLOAT64 StringRecorder::callback( PELEMENT_STRING pelement ) {	
	//dout << "> callback " << d->stringHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	if( (pelement->string==0 && d->oldValue[0]!=0) 
		|| strcmp(pelement->string, d->oldValue)!=0 ) {
		dout << "String callback " << d->stringHeader << ":" << this << " in " << gauge().name();
		
		// empty string?
		if( pelement->string==0 ) {
			d->oldValue[0] = 0;
		} else {
			int len = strlen( pelement->string );
			if( len<32 )
				strcpy( d->oldValue, pelement->string );
			else {
				strncpy( d->oldValue, pelement->string, 31 );
				d->oldValue[31]=0;
			}
		}
		dout << " = " << (unsigned long)ret << std::endl;
		gauge().send( new StringUpdatePacket( "", 0, id(), d->oldValue ), true );
	}
	//dout << "< cal lback " << d->stringHeader << std::endl;
	return ret;
}


/****************************************************************************************/

StringViewer::StringViewer( int id, Gauge &gauge, ELEMENT_STRING *stringHeader )
	: StringElement( id, gauge, stringHeader ) {
}

StringViewer::~StringViewer() {
}

FLOAT64 StringViewer::callback( PELEMENT_STRING pelement ) {
	FLOAT64 ret = (*d->origCallback)( pelement );
	pelement->string = d->buffer;
	strcpy( d->buffer, d->oldValue );
	return ret;
}


void StringViewer::receive( UpdatePacket *packet ) {
	StringUpdatePacket *iup= (StringUpdatePacket*)packet;
	strcpy( d->oldValue, iup->value );
}
