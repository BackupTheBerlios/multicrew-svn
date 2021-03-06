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


/**********************************************************************/
struct StringElement::Data {	
	Data( StringElement *el ) 
		: callbackAdapter( el, StringElement::callback ) {
	}
	ELEMENT_STRING *stringHeader;
	PSTRING_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, StringElement, PELEMENT_STRING> callbackAdapter;

	char value[BUFFER_SIZE+1];
	char buffer[BUFFER_SIZE+1];
	CRITICAL_SECTION cs;
	bool changed;
};


StringElement::StringElement( Gauge *gauge, unsigned num )
	: Element( gauge, num ), NetworkChannel( id() ) {
	d = new Data( this );
	d->stringHeader = 0;
	d->value[0] = 0;
	d->value[BUFFER_SIZE] = 0;
	d->buffer[0] = 0;
	d->buffer[BUFFER_SIZE] = 0;
	d->origCallback = 0;
	d->changed = false;

	InitializeCriticalSection( &d->cs );
}


StringElement::~StringElement() {
	detach();
	DeleteCriticalSection( &d->cs );
	delete d;
}


void StringElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->stringHeader = (ELEMENT_STRING*)elementHeader;
	d->origCallback = d->stringHeader->update_cb;

	if( d->stringHeader->update_cb!=NULL ) {
		// install callback wrapper	
//		dout << "install updateCallback for " << d->stringHeader << std::endl;
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


SmartPtr<PacketBase> StringElement::createPacket( SharedBuffer &buffer ) {
	return new StringPacket( buffer );
}


void StringElement::sendState() {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		EnterCriticalSection( &d->cs );
		send( new StringPacket(d->value), false, highPriority );
		LeaveCriticalSection( &d->cs );
	} break;
	case MulticrewCore::ClientMode: break;
	}
}


void StringElement::receive( SmartPtr<PacketBase> packet ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<StringPacket> sp = (StringPacket*)&*packet;
		EnterCriticalSection( &d->cs );
		strncpy( d->value, sp->string.c_str(), BUFFER_SIZE );
		LeaveCriticalSection( &d->cs );
		dout << "String receive " << id() << " = " << d->value << std::endl;
		d->changed = true;
	} break;
	}
}


void StringElement::sendFullState() {
	sendState();
}


FLOAT64 StringElement::callback( PELEMENT_STRING pelement ) {	
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode:
		return (*d->origCallback)( pelement );
	case MulticrewCore::HostMode:
	{ 
		//dout << "> callback " << d->stringHeader << std::endl;
		FLOAT64 ret = (*d->origCallback)( pelement );
		EnterCriticalSection( &d->cs );
		if( (pelement->string==0 && d->value[0]!=0) 
			|| strcmp(pelement->string, d->value)!=0 ) {	
			// empty string?
			if( pelement->string==0 )
				d->value[0] = 0;
			else
				strncpy( d->value, pelement->string, BUFFER_SIZE );
			dout << "String callback " << id() << " = " << d->value << std::endl;			
			sendState();
		}
		//dout << "< cal lback " << d->stringHeader << std::endl;
		LeaveCriticalSection( &d->cs );
		return ret;
	} break;
	case MulticrewCore::ClientMode:
	{
		EnterCriticalSection( &d->cs );
		FLOAT64 ret = (*d->origCallback)( pelement );
		pelement->string = d->buffer;
		strncpy( d->buffer, d->value, BUFFER_SIZE );	
		LeaveCriticalSection( &d->cs );

		if( d->changed ) {
			d->changed = false;

            // redraw statics		
			if( d->stringHeader->image_flags & IMAGE_USE_TRANSPARENCY ) {
				gauge()->redrawStatics();
			}
			SET_OFF_SCREEN( d->stringHeader );
		}
		return ret;
	} break;
	}

	return 0.0;
}
