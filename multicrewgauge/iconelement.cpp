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


#define ICON_FLAGS_MASK (~(IMAGE_ON_SCREEN |IMAGE_HIDDEN ))

struct IconStruct {
	FLOAT64 value;
	FLAGS flags;
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
	FLAGS oldFlags;
	bool changed;
};


IconElement::IconElement( Gauge *gauge, unsigned num )
	: Element( gauge, num ), NetworkChannel( id() ) {
	d = new Data( this );
	d->iconHeader = 0;
	d->oldValue = 0.0;
	d->origCallback = 0;
	d->changed = false;
}


IconElement::~IconElement() {
	detach();
	delete d;
}


void IconElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->iconHeader = (ELEMENT_ICON*)elementHeader;
	d->origCallback = d->iconHeader->update_cb;
	d->oldFlags = d->iconHeader->image_flags & ICON_FLAGS_MASK;

	if( d->iconHeader->update_cb==NULL ) {
		dout << id() << " no update_cb " << std::endl;
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


void IconElement::receive( SmartPtr<PacketBase> packet ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<IconPacket> ip = (IconPacket*)&*packet;
		IconStruct &icon = ip->data();
		d->oldValue = icon.value;
		d->oldFlags = icon.flags & ICON_FLAGS_MASK;
		//dout << "Icon receive " << id() << " = " << (unsigned long)d->oldValue << std::endl;
		d->changed = true;		
	} break;
	}
}


SmartPtr<PacketBase> IconElement::createPacket( SharedBuffer &buffer ) {
	return new IconPacket( buffer );
}


void IconElement::sendFullState() {
	sendState();
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
			
		if( ret!=d->oldValue || (pelement->image_flags & ICON_FLAGS_MASK)!=d->oldFlags ) {
			dout << "Icon callback " << id() 
				 << " = " << (unsigned long)ret
				 << "/" << to_string( pelement->image_flags, 2 )
				 << std::endl;
			d->oldValue = ret;
			d->oldFlags = pelement->image_flags & ICON_FLAGS_MASK;
			sendState();
		}
		return ret;
	} break;
	case MulticrewCore::ClientMode: {
		pelement->image_flags = (pelement->image_flags & ~ICON_FLAGS_MASK) | d->oldFlags;
		if( d->origCallback )
			(*d->origCallback)( pelement );

		if( d->changed ) {
			dout << "Icon redraw " << id() 
				 << " = " << (unsigned long)d->oldValue << std::endl;
			d->changed = false;

            // redraw statics		
			if( d->iconHeader->image_flags & IMAGE_USE_TRANSPARENCY ) {
				gauge()->redrawStatics();
			}
			SET_OFF_SCREEN( d->iconHeader );
		}
		return d->oldValue;
	} break;
	}

	return 0.0;
}


void IconElement::sendState() {	
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		IconStruct s;
		s.value = d->oldValue;
		s.flags = d->oldFlags;
		send( new IconPacket( s ), false, highPriority );
	} break;
	case MulticrewCore::ClientMode: break;
	}   
}
