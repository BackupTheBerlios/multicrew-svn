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

struct StaticElement::Data {		
	ELEMENT_STATIC_IMAGE *staticHeader;	
};

StaticElement::StaticElement( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader )
	: Element( id, gauge, (ELEMENT_HEADER*)staticHeader ) {
	d = new Data;
	d->staticHeader = staticHeader;	
}

StaticElement::~StaticElement() {	
	delete d;
}

ELEMENT_STATIC_IMAGE *StaticElement::staticHeader() { 
	return d->staticHeader; 
}

/****************************************************************************************/

StaticRecorder::StaticRecorder( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader )
	: StaticElement( id, gauge, staticHeader ) {
}

StaticRecorder::~StaticRecorder() {
}


/****************************************************************************************/

StaticViewer::StaticViewer( int id, Gauge &gauge, ELEMENT_STATIC_IMAGE *staticHeader )
	: StaticElement( id, gauge, staticHeader ) {
}

StaticViewer::~StaticViewer() {
}
