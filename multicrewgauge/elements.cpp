/*
Multicrew
Copyright (C) 2005 Stefan Schimanski

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

#include <map>

#include "multicrewgauge.h"
#include "../multicrewcore/streams.h"
#include "gauges.h"


struct Element::Data {
	Gauge *gauge;	
	ELEMENT_HEADER *elementHeader;
	int id;
};

Element::Element( int id, Gauge &gauge ) {
	d = new Data;
	d->gauge = &gauge;
	d->elementHeader = 0;
	d->id = id;
	//dout << "Element " << elementHeader << std::endl;	
}

Element::~Element() {	
	delete d;
}

ELEMENT_HEADER *Element::elementHeader() { 
	return d->elementHeader; 
}

Gauge &Element::gauge() { 
	return *d->gauge; 
}

int Element::id() {
	return d->id;
}

void Element::attach( ELEMENT_HEADER *elementHeader ) {
	d->elementHeader = elementHeader;
}

void Element::detach() {
	d->elementHeader = 0;
}
