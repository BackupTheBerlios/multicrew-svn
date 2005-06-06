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

#include "multicrewcore.h"
#include "streams.h"
#include "log.h"
    

/*******************************************************************/
struct MulticrewModule::Data {
	Data( MulticrewModule *mod ) {
	}

	SmartPtr<MulticrewCore> core;
	SmartPtr<FileConfig> config;
};


MulticrewModule::MulticrewModule( std::string moduleName ) 
	: Identified( moduleName ) {
	d = new Data( this );
	d->core = MulticrewCore::multicrewCore();
	
	MulticrewCore::multicrewCore()->registerModule( this );
}


MulticrewModule::~MulticrewModule() {
	dout << "~MulticrewModule()" << std::endl;
	MulticrewCore::multicrewCore()->unregisterModule( this );
	delete d;
}


SmartPtr<FileConfig> MulticrewModule::config() {
	if( d->config.isNull() )
		d->config = new FileConfig( "multicrew/" + id() + ".ini" );
	SmartPtr<FileConfig> ret = d->config;   
	return ret;
}
