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

#define _WIN32_DCOM 
#include <objbase.h>

#include <list>
#include <deque>
#include <map>

#include "streams.h"
#include "signals.h"
#include "multicrewcore.h"
#include "position.h"
#include "log.h"
#include "fsuipcmodule.h"

static MulticrewCore *multicrewCore = 0;

SmartPtr<MulticrewCore> MulticrewCore::multicrewCore() {
	if( ::multicrewCore==0 ) ::multicrewCore = new MulticrewCore();
	// dout << "MulticrewCore::multicrewCore()" << std::endl;
	return ::multicrewCore;
}


struct MulticrewCore::Data {	
	bool hostMode;

	std::map<std::string, MulticrewModule*> modules;
	SmartPtr<PositionModule> posModule;
	SmartPtr<FsuipcModule> fsuipcModule;

	__int64 perfTimerFreq;
	__int64 startTime;
};


MulticrewCore::MulticrewCore() {
	d = new Data;
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	dout << "MulticrewCore" << std::endl;

	if( !QueryPerformanceFrequency((LARGE_INTEGER*)&d->perfTimerFreq) )
		dlog << "No performance timer available" << std::endl;
	QueryPerformanceCounter( (LARGE_INTEGER*)&d->startTime );
}


MulticrewCore::~MulticrewCore() {
	dout << "~MulticrewCore" << std::endl;
	d->modules.clear();
	::multicrewCore = 0;
	CoUninitialize();
	delete d;
}


double MulticrewCore::time() {
	__int64 now;
	QueryPerformanceCounter( (LARGE_INTEGER*)&now );
	return ((double)now-d->startTime)/d->perfTimerFreq;
}


bool MulticrewCore::registerModule( MulticrewModule *module ) {
	dout << "module " << module->moduleName() << " registered" << std::endl;
	if( d->modules.empty() ) {
		// first module sets hostMode
		d->hostMode = module->isHostMode();
	} else {
		// valid hostMode?
		if( module->isHostMode()!=d->hostMode ) {
			derr << "Host and client mode cannot be mixed which is the case for \"" 
				 << module->moduleName().c_str() << "\"" << std::endl;
			return false;
		}
	}

	// register module
	d->modules[module->moduleName().c_str()] = module;	

	// first module?
	if( d->modules.size()==1 ) {	
		// position module (automatically registers to the modules list)
		if( d->hostMode )
			d->posModule = new PositionHostModule();
		else
		d->posModule = new PositionClientModule();

		// FSUIPC module
		d->fsuipcModule = new FsuipcModule( d->hostMode );
		d->fsuipcModule->watch( 0xbdc, 4, true ); // flaps control
		d->fsuipcModule->watch( 0xbd0, 4, false ); // spoiler control
		d->fsuipcModule->watch( 0xbb2, 2, false ); // elevator control
		d->fsuipcModule->watch( 0xbb6, 2, false ); // ailerons control
		d->fsuipcModule->watch( 0xbba, 2, false ); // rudder control
		d->fsuipcModule->watch( 0xbc8, 2, true ); // parking brake
		d->fsuipcModule->watch( 0xbe8, 4, true, true ); // gears commanded
		d->fsuipcModule->watch( 0x88c, 8, true ); // engine 1
		d->fsuipcModule->watch( 0x924, 8, true ); // engine 2
		d->fsuipcModule->watch( 0x3bfc, 4, true ); // ZFW
		d->fsuipcModule->watch( 0x280, 2, true ); // nav + strobe lights 
		d->fsuipcModule->watch( 0x28c, 2, true ); // landing lights
		d->fsuipcModule->watch( 0xd0c, 4, true ); // light (FS2000+)
		//d->fsuipcModule->watch( 0x560, 36, false ); // position
		//d->fsuipcModule->watch( 0x3060, 6*8 ); // acceleration
		//d->fsuipcModule->watch( 0x3090, 6*8, false ); // velocity
		d->fsuipcModule->watch( 0x281c, 2, true ); // master battery switch
		d->fsuipcModule->watch( 0x2e80, 4, true ); // avionics master switch
		d->fsuipcModule->watch( 0x3100, 4, true ); // alternator, battery, avionics
		d->fsuipcModule->watch( 0x2e80, 4, true ); // avionics master switch
		d->fsuipcModule->watch( 0x3125, 4, true ); // fuel pumps

		// emit signal
		planeLoaded.emit();
	}

	return true;
}


void MulticrewCore::unregisterModule( MulticrewModule *module ) {
	dout << "module " << module->moduleName() << " unregistered" << std::endl;

	// find and remove module from module list
	std::map<std::string, MulticrewModule*>::iterator res = d->modules.find( module->moduleName().c_str() );
	if( res!=d->modules.end() ) {
		d->modules.erase( res );
		if( d->modules.empty() ) {
			// destroy FSUIPC module
			d->posModule = 0;
			d->fsuipcModule = 0;

			// no modules registered anymore
			dout << "unload" << std::endl;
			planeUnloaded.emit();
		}
	}
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
	// prepare modules for connection
	std::map<std::string, MulticrewModule*>::iterator it = d->modules.begin();
	while( it!=d->modules.end() ) {
		it->second->connect( con );
		it++;
	}
}


void MulticrewCore::unprepare() {
	// unconnect modules from connection
	std::map<std::string, MulticrewModule*>::iterator it = d->modules.begin();
	while( it!=d->modules.end() ) {
		it->second->disconnect();
		it++;
	}
}
