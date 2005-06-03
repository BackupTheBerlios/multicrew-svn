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


#define STATIC_FLAGS_MASK (~(IMAGE_HIDDEN | IMAGE_ON_SCREEN))
						   


struct StaticStruct {
	FLAGS flags;
};

typedef StructPacket<StaticStruct> StaticPacket;


struct StaticElement::Data {	
	Data( StaticElement *el ) {
	}
	ELEMENT_STATIC_IMAGE *staticHeader;
	FLAGS oldFlags;
	bool watchFlags;
};


StaticElement::StaticElement( Gauge *gauge, unsigned num, bool watchFlags )
	: Element( gauge, num ), NetworkChannel( id() ) {
	d = new Data( this );
	d->staticHeader = 0;
	d->watchFlags = watchFlags;
}


StaticElement::~StaticElement() {
	detach();
	delete d;
}


void StaticElement::attach( ELEMENT_HEADER *elementHeader ) {
	Element::attach( elementHeader );
	d->staticHeader = (ELEMENT_STATIC_IMAGE*)elementHeader;
	d->oldFlags = d->staticHeader->image_flags & STATIC_FLAGS_MASK;
}


void StaticElement::detach() {
	Element::detach();
	if( d->staticHeader ) {
		d->staticHeader = 0;
	}
}


void StaticElement::receive( SmartPtr<PacketBase> packet ) {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<StaticPacket> ip = (StaticPacket*)&*packet;
		StaticStruct &s = ip->data();
		d->oldFlags = s.flags & STATIC_FLAGS_MASK;
		dout << "Static receive " << id() 
			 << " = " << to_string( d->oldFlags, 2 ) 
			 << std::endl;
	} break;
	}
}


SmartPtr<PacketBase> StaticElement::createPacket( SharedBuffer &buffer ) {
	return new StaticPacket( buffer );
}


void StaticElement::sendFullState() {
	sendState();
}


void StaticElement::redraw() {
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: break;
	case MulticrewCore::ClientMode: {
		//dout << "Redrawing static " << id() << std::endl;
		SET_OFF_SCREEN( d->staticHeader ); 
	} break;
	}
}

void StaticElement::callback() {
	if( d->staticHeader==0 || !d->watchFlags) return;

	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:		
		if( (d->staticHeader->image_flags & STATIC_FLAGS_MASK)!=d->oldFlags ) {
			dout << "Static callback " << id() 
				 << " = " << to_string( d->staticHeader->image_flags, 2 )
				 << std::endl;
			d->oldFlags = d->staticHeader->image_flags & STATIC_FLAGS_MASK;
			sendState();
		}
		break;
	case MulticrewCore::ClientMode:
		d->staticHeader->image_flags = (d->staticHeader->image_flags & ~STATIC_FLAGS_MASK) | d->oldFlags;		
		break;
	}
}


void StaticElement::sendState() {	
	switch( core()->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	{
		StaticStruct s;
		s.flags = d->oldFlags;
		send( new StaticPacket( s ), false, highPriority );
	} break;
	case MulticrewCore::ClientMode: break;
	}   
}
