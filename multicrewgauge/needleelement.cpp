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


/************************ packets *****************************/
#pragma pack(push,1)
struct NeedleStruct {
	FLOAT64 value;
};
#pragma pack(pop)

typedef StructPacket<NeedleStruct> NeedlePacket;


/**************************************************************/
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


NeedleElement::NeedleElement( Gauge *gauge, unsigned num )
	: Element( gauge, num ), NetworkChannel( id() ) {
	d = new Data( this );
	d->needleHeader = 0;
	d->oldValue = (FLOAT64)0x57524320;
	d->oldValue = 0.0;
	d->origCallback = 0;
	d->changed = false;
}


NeedleElement::~NeedleElement() {
	detach();
	delete d;
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


SmartPtr<PacketBase> NeedleElement::createPacket( SharedBuffer &buffer ) {
	return new NeedlePacket( buffer );
}


void NeedleElement::sendState() {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		NeedleStruct s;
		s.value = d->oldValue;
		send( new NeedlePacket( s ), false, mediumPriority );
	} break;
	case MulticrewCore::ClientMode: break;
	}   
}


void NeedleElement::receive( SmartPtr<PacketBase> packet ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<NeedlePacket> ip = (NeedlePacket*)&*packet;
		d->oldValue = ip->data().value;
		d->changed = true;
	} break;
	}
}


void NeedleElement::sendFullState() {
	sendState();
}


FLOAT64 NeedleElement::callback( PELEMENT_NEEDLE pelement ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode:
		return (*d->origCallback)( pelement );
	case MulticrewCore::HostMode:
	{ 
        //dout << "> callback " << d->needleHeader << std::endl;
		FLOAT64 ret = (*d->origCallback)( pelement );
		if( ret!=d->oldValue ) {
			//dout << "Needle callback " << d->needleHeader << ":" << this 
			//	 << " in " << gauge().name() << " = " << (unsigned long)ret << std::endl;
			d->oldValue = ret;
			sendState();
		}
		//dout << "< cal lback " << d->needleHeader << std::endl;
		return ret;
	} break;
	case MulticrewCore::ClientMode:
	{
		(*d->origCallback)( pelement );

		if( d->changed ) {
			d->changed = false;

            // redraw statics		
			if( d->needleHeader->image_flags & IMAGE_USE_TRANSPARENCY ) {
				gauge()->redrawStatics();
			}
			SET_OFF_SCREEN( d->needleHeader );
		}

		return d->oldValue;
	} break;
	}

	return 0.0;
}
