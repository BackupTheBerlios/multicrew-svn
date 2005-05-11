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

	/* recorder */
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


void IconElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->iconHeader = (ELEMENT_ICON*)elementHeader;
	d->origCallback = d->iconHeader->update_cb;

	if( d->iconHeader->update_cb==NULL ) {
		dout << gauge().name() << ":" << id() << " no update_cb " << std::endl;
	}
	
	d->iconHeader->update_cb = d->callbackAdapter.callback();
}


void IconElement::detach() {
	Element::detach();
	if( d->iconHeader ) {
		d->iconHeader->update_cb = d->origCallback;	
		d->iconHeader = 0;
	}
	d->origCallback = 0;
}


SmartPtr<PacketBase> IconElement::createPacket( SharedBuffer &buffer ) {
	return new IconPacket( buffer );
}


FLOAT64 IconElement::callback( PELEMENT_ICON pelement ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: 
		if( d->origCallback )
			return (*d->origCallback)( pelement );
		else
			if( pelement->source_var.id!=-1 ) return pelement->source_var.var_value.n;
		break;
	case MulticrewCore::HostMode: {
		FLOAT64 ret = 0.0;
		if( d->origCallback )
			ret = (*d->origCallback)( pelement );
		else
			if( pelement->source_var.id!=-1 ) ret = pelement->source_var.var_value.n;
			
		if( ret!=d->oldValue ) {
			dout << "Icon callback " << d->iconHeader << ":" << this 
				 << " in " << gauge().name() << " = " << (unsigned long)ret 
				 << " id=" << id() << std::endl;
			d->oldValue = ret;
			gauge().requestSend( this );
		}
		// dout << "< callback " << d->iconHeader << std::endl;
		return ret;
	} break;
	case MulticrewCore::ClientMode: {
		if( d->origCallback )
			(*d->origCallback)( pelement );
		return d->oldValue;
	}
	}

	return 0.0;
}


void IconElement::sendProc() {	
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		IconStruct s;
		s.value = d->oldValue;
		gauge().send( id(), new IconPacket( s ), true );
	} break;
	case MulticrewCore::ClientMode: break;
	}   
}


void IconElement::receive( SmartPtr<PacketBase> packet ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		dout << "receive icon packet" << std::endl;
		SmartPtr<IconPacket> ip = (IconPacket*)&*packet;
		d->oldValue = ip->data().value;
	} break;
	}
}
