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

#include "multicrewcore.h"

struct MulticrewModule::Data {
	std::string moduleName;
	bool hostMode;
	SmartPtr<Connection> con;
};

MulticrewModule::MulticrewModule( std::string moduleName, bool hostMode ) {
	d = new Data;
	d->moduleName = moduleName;
	d->hostMode = hostMode;
}


MulticrewModule::~MulticrewModule() {
	delete d;
}


std::string MulticrewModule::moduleName() {
	return d->moduleName;
}


bool MulticrewModule::isHostMode() {
	return d->hostMode;
}


void MulticrewModule::send( ModulePacket *packet, bool safe, bool sync ) {  
	if( !d->con.isNull() ) {
		strcpy( packet->module, moduleName().c_str() );
		//dout << "Next packet for " << packet->module << std::endl;
		d->con->send( packet, safe, sync );
	}
}


std::string MulticrewModule::id() {
	return d->moduleName;
}


void MulticrewModule::connect( SmartPtr<Connection> con ) {
	d->con = con;
	d->con->addReceiver( this );
}


void MulticrewModule::disconnect() {
	d->con->removeReceiver( this );
	d->con = 0;
}


