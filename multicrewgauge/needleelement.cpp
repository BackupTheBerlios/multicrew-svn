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

struct NeedleElement::Data {	
	Data( NeedleElement *el ) 
		: callbackAdapter( el, NeedleElement::callback ) {
	}
	ELEMENT_NEEDLE *needleHeader;
	PNEEDLE_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, NeedleElement, PELEMENT_NEEDLE> callbackAdapter;

	FLOAT64 oldValue;
};

NeedleElement::NeedleElement( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader )
	: Element( id, gauge, (ELEMENT_HEADER*)needleHeader ) {
	d = new Data( this );
	d->needleHeader = needleHeader;
	d->origCallback = needleHeader->update_cb;
	d->oldValue = (FLOAT64)0x57524320;
	d->oldValue = 0.0;

	// debug code for MasterCaution gauge
	if( /*gauge.name()=="MasterCaution" &&*/ d->needleHeader->update_cb!=NULL ) {
		// install callback wrapper	
		dout << "install updateCallback for " << d->needleHeader << std::endl;
		d->needleHeader->update_cb = d->callbackAdapter.callback();
	}
}

NeedleElement::~NeedleElement() {
	//d->needleHeader->update_cb = d->origUpdateCallback;	
	delete d;
}

ELEMENT_NEEDLE *NeedleElement::needleHeader() { 
	return d->needleHeader; 
}

/****************************************************************************************/

NeedleRecorder::NeedleRecorder( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader )
	: NeedleElement( id, gauge, needleHeader ) {
}

NeedleRecorder::~NeedleRecorder() {
}

FLOAT64 NeedleRecorder::callback( PELEMENT_NEEDLE pelement ) {	
	//dout << "> callback " << d->needleHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	if( ret!=d->oldValue ) {
		dout << "Needle callback " << d->needleHeader << ":" << this << " in " << gauge().name();
		d->oldValue = ret;
		dout << " = " << (unsigned long)ret << std::endl;
		gauge().send( new NeedleUpdatePacket( "", 0, id(), ret ), true );
	}
	//dout << "< cal lback " << d->needleHeader << std::endl;
	return ret;
}


/*******************************************************************************/

NeedleViewer::NeedleViewer( int id, Gauge &gauge, ELEMENT_NEEDLE *needleHeader )
	: NeedleElement( id, gauge, needleHeader ) {
}

NeedleViewer::~NeedleViewer() {
}

FLOAT64 NeedleViewer::callback( PELEMENT_NEEDLE pelement ) {
	FLOAT64 ret = (*d->origCallback)( pelement );
	return d->oldValue;
}


void NeedleViewer::receive( UpdatePacket *packet ) {
	NeedleUpdatePacket *iup= (NeedleUpdatePacket*)packet;
	d->oldValue = iup->value;
}
