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


struct IconStruct {
	FLOAT64 value;
};

typedef StructPacket<IconStruct> IconPacket;


struct IconElement::Data {	
	Data( IconElement *el ) 
		: callbackAdapter( el, IconElement::callback ) {
	}
	ELEMENT_ICON *iconHeader;
	PICON_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, IconElement, PELEMENT_ICON> callbackAdapter;

	FLOAT64 oldValue;
};


IconElement::IconElement( int id, Gauge &gauge )
	: Element( id, gauge ) {
	d = new Data( this );
	d->iconHeader = 0;
	d->oldValue = 0.0;
	d->origCallback = 0;
}

IconElement::~IconElement() {
	detach();
	delete d;
}

ELEMENT_ICON *IconElement::iconHeader() { 
	if( d->iconHeader ) d->iconHeader->update_cb = d->origCallback;	
	return d->iconHeader; 
}

void IconElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->iconHeader = (ELEMENT_ICON*)elementHeader;
	d->origCallback = d->iconHeader->update_cb;

	if( d->iconHeader->update_cb!=NULL ) {
		// install callback wrapper	
//		dout << "install updateCallback for " << d->iconHeader << std::endl;
		d->iconHeader->update_cb = d->callbackAdapter.callback();
	}
}

void IconElement::detach() {
	Element::detach();
	if( d->iconHeader ) {
		d->iconHeader->update_cb = d->origCallback;	
		d->iconHeader = 0;
	}
	d->origCallback = 0;
}


SmartPtr<Packet> IconElement::createPacket( SharedBuffer &buffer ) {
	return new IconPacket( buffer );
}


/****************************************************************************************/


IconRecorder::IconRecorder( int id, Gauge &gauge )
	: IconElement( id, gauge ) {
}

IconRecorder::~IconRecorder() {
}

FLOAT64 IconRecorder::callback( PELEMENT_ICON pelement ) {	
	//dout << "> callback " << d->iconHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	if( ret!=d->oldValue ) {
		dout << "Icon callback " << d->iconHeader << ":" << this 
			 << " in " << gauge().name() << " = " << (unsigned long)ret 
			 << " id=" << id() << std::endl;
		d->oldValue = ret;
		gauge().requestSend( this );
	}
	// dout << "< cal lback " << d->iconHeader << std::endl;
	return ret;
}


void IconRecorder::sendProc() {
	IconStruct s;
	s.value = d->oldValue;
	gauge().send( id(), new IconPacket( s ), true );
}


/****************************************************************************************/

IconViewer::IconViewer( int id, Gauge &gauge )
	: IconElement( id, gauge ) {
}

IconViewer::~IconViewer() {
}

FLOAT64 IconViewer::callback( PELEMENT_ICON pelement ) {
	FLOAT64 ret = (*d->origCallback)( pelement );
	return d->oldValue;
}


void IconViewer::receive( SmartPtr<Packet> packet ) {
	SmartPtr<IconPacket> ip = (IconPacket*)&*packet;
	d->oldValue = ip->data().value;
}
