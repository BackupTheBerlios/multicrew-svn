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

struct IconElement::Data {	
	Data( IconElement *el ) 
		: callbackAdapter( el, IconElement::callback ) {
	}
	ELEMENT_ICON *iconHeader;
	PICON_UPDATE_CALLBACK origCallback;
	CallbackAdapter1<FLOAT64, IconElement, PELEMENT_ICON> callbackAdapter;

	FLOAT64 oldValue;
};

IconElement::IconElement( int id, Gauge &gauge, ELEMENT_ICON *iconHeader )
	: Element( id, gauge, (ELEMENT_HEADER*)iconHeader ) {
	d = new Data( this );
	d->iconHeader = iconHeader;
	d->origCallback = iconHeader->update_cb;
	d->oldValue = 0.0;

	// debug code for MasterCaution gauge
	if( /*gauge.name()=="MasterCaution" &&*/ d->iconHeader->update_cb!=NULL ) {
		// install callback wrapper	
		dout << "install updateCallback for " << d->iconHeader << std::endl;
		d->iconHeader->update_cb = d->callbackAdapter.callback();
	}
}

IconElement::~IconElement() {
	//d->iconHeader->update_cb = d->origUpdateCallback;	
	delete d;
}

ELEMENT_ICON *IconElement::iconHeader() { 
	return d->iconHeader; 
}

/****************************************************************************************/

IconRecorder::IconRecorder( int id, Gauge &gauge, ELEMENT_ICON *iconHeader )
	: IconElement( id, gauge, iconHeader ) {
}

IconRecorder::~IconRecorder() {
}

FLOAT64 IconRecorder::callback( PELEMENT_ICON pelement ) {	
	//dout << "> callback " << d->iconHeader << std::endl;
	FLOAT64 ret = (*d->origCallback)( pelement );
	if( ret!=d->oldValue ) {
		dout << "Icon callback " << d->iconHeader << ":" << this << " in " << gauge().name();
		d->oldValue = ret;
		dout << " = " << (unsigned long)ret << std::endl;
		gauge().send( new IconUpdatePacket( "", 0, id(), ret ), true );
	}
	//dout << "< cal lback " << d->iconHeader << std::endl;
	return ret;
}


/****************************************************************************************/

IconViewer::IconViewer( int id, Gauge &gauge, ELEMENT_ICON *iconHeader )
	: IconElement( id, gauge, iconHeader ) {
}

IconViewer::~IconViewer() {
}

FLOAT64 IconViewer::callback( PELEMENT_ICON pelement ) {
	FLOAT64 ret = (*d->origCallback)( pelement );
	return d->oldValue;
}


void IconViewer::receive( UpdatePacket *packet ) {
	IconUpdatePacket *iup= (IconUpdatePacket*)packet;
	d->oldValue = iup->value;
}
