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

#include <list>
#include <deque>
#include <map>

#include "debug.h"
#include "error.h"
#include "signals.h"
#include "multicrewcore.h"
#include "position.h"

static MulticrewCore *multicrewCore = 0;

SmartPtr<MulticrewCore> MulticrewCore::multicrewCore() {
	if( ::multicrewCore==0 ) ::multicrewCore = new MulticrewCore();
	dout << "MulticrewCore::multicrewCore()" << std::endl;
	return ::multicrewCore;
}


struct MulticrewCore::Data {	
	bool hostMode;

	std::map<std::string, MulticrewModule*> modules;
	PositionModule *posModule;
};


MulticrewCore::MulticrewCore() {
	d = new Data;
	d->posModule = 0;
	dout << "MulticrewCore" << std::endl;
}


MulticrewCore::~MulticrewCore() {
	dout << "~MulticrewCore" << std::endl;
	delete d->posModule;
	d->modules.clear();
	::multicrewCore = 0;
	delete d;
}


bool MulticrewCore::registerModule( MulticrewModule *module ) {
	dout << "> registerModule" << std::endl;
	if( d->modules.empty() ) {
		// first module sets hostMode
		d->hostMode = module->isHostMode();
	} else {
		// valid hostMode?
		if( module->isHostMode()!=d->hostMode ) {
			derr << "Host and client mode cannot be mixed which is the case for \"" << module->moduleName().c_str() << "\"" << std::endl;
			return false;
		}
	}

	// register module
	d->modules[module->moduleName().c_str()] = module;	

	// first module?
	if( d->modules.size()==1 ) {
		// create FSUIPC object
		if( d->hostMode )
			d->posModule = new PositionHostModule();
		else
			d->posModule = new PositionClientModule();
		d->posModule->start();
		
		// emit signal
		planeLoaded.emit();
	}

	dout << "< registerModule" << std::endl;

	return true;
}


void MulticrewCore::unregisterModule( MulticrewModule *module ) {
	dout << "> unregisterModule" << std::endl;
	std::map<std::string, MulticrewModule*>::iterator res = d->modules.find( module->moduleName().c_str() );
	if( res!=d->modules.end() ) {
		d->modules.erase( res );
		if( d->modules.empty() ) {
			dout << "unload" << std::endl;
			planeUnloaded.emit();

			// destroy position module
			delete d->posModule; d->posModule=0;
		}
	}
	dout << "< unregisterModule" << std::endl;
}


bool MulticrewCore::isPlaneLoaded() {
	return d->modules.size()>0;
}


bool MulticrewCore::isHostMode() {
	return d->hostMode;
}


void MulticrewCore::log( std::string line ) {
	logged.emit( line.c_str() );
}


void MulticrewCore::prepare( SmartPtr<Connection> con ) {
	std::map<std::string, MulticrewModule*>::iterator it = d->modules.begin();
	while( it!=d->modules.end() ) {
		it->second->connect( con );
		it++;
	}
}


void MulticrewCore::unprepare() {
	std::map<std::string, MulticrewModule*>::iterator it = d->modules.begin();
	while( it!=d->modules.end() ) {
		it->second->disconnect();
		it++;
	}
}
