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


struct NeedleStruct {
	FLOAT64 value;
};

typedef StructPacket<NeedleStruct> NeedlePacket;


struct NeedleElement::Data {	
	Data( NeedleElement *el ) 
		: callbackAdapter( el, NeedleElement::callback ) {
	}
	ELEMENT_NEEDLE *needleHeader;
	PNEEDLE_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, NeedleElement, PELEMENT_NEEDLE> callbackAdapter;

	FLOAT64 oldValue;
	bool changed;
};

NeedleElement::NeedleElement( int id, Gauge &gauge )
	: Element( id, gauge ) {
	d = new Data( this );
	d->needleHeader = 0;
	d->oldValue = (FLOAT64)0x57524320;
	d->oldValue = 0.0;
	d->changed = true;
	d->origCallback = 0;
}

NeedleElement::~NeedleElement() {
	detach();
	delete d;
}

ELEMENT_NEEDLE *NeedleElement::needleHeader() { 
	return d->needleHeader; 
}

void NeedleElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->needleHeader = (ELEMENT_NEEDLE*)elementHeader;
	d->origCallback = d->needleHeader->update_cb;

	if( d->needleHeader->update_cb!=NULL ) {
		// install callback wrapper	
		//dout << "install updateCallback for " << d->needleHeader << std::endl;
		d->needleHeader->update_cb = d->callbackAdapter.callback();
	}
}

void NeedleElement::detach() {
	Element::detach();
	if( d->needleHeader ) {
		d->needleHeader->update_cb = d->origCallback;	
		d->needleHeader = 0;
	}
}


SmartPtr<Packet> NeedleElement::createPacket( SharedBuffer &buffer ) {
	return new NeedlePacket( buffer );
}


/****************************************************************************************/

NeedleRecorder::NeedleRecorder( int id, Gauge &gauge )
	: NeedleElement( id, gauge ) {
}

NeedleRecorder::~NeedleRecorder() {
}

FLOAT64 NeedleRecorder::callback( PELEMENT_NEEDLE pelement ) {	
	//dout << "> callback " << d->needleHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	if( ret!=d->oldValue ) {
		dout << "Needle callback " << d->needleHeader << ":" << this 
			 << " in " << gauge().name() << " = " << (unsigned long)ret << std::endl;
		d->oldValue = ret;
		d->changed = true;
	}
	//dout << "< cal lback " << d->needleHeader << std::endl;
	return ret;
}


void NeedleRecorder::sendProc() {
	if( d->changed ) {
		NeedleStruct s;
		s.value = d->oldValue;
		gauge().send( id(), new NeedlePacket( s ), true );
		d->changed = false;
	}
}

/*******************************************************************************/

NeedleViewer::NeedleViewer( int id, Gauge &gauge )
	: NeedleElement( id, gauge ) {
}

NeedleViewer::~NeedleViewer() {
}

FLOAT64 NeedleViewer::callback( PELEMENT_NEEDLE pelement ) {
	FLOAT64 ret = (*d->origCallback)( pelement );
	return d->oldValue;
}


void NeedleViewer::receive( SmartPtr<Packet> packet ) {
	SmartPtr<NeedlePacket> ip = (NeedlePacket*)&*packet;
	d->oldValue = ip->data().value;
}
