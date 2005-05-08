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
#include <set>
#include <vector>
#include <sstream>

#include "../stlplus/source/string_utilities.hpp"

#include "streams.h"
#include "signals.h"
#include "multicrewcore.h"
#include "position.h"
#include "log.h"
#include "config.h"
#include "fsuipcmodule.h"

static MulticrewCore *multicrewCore = 0;

SmartPtr<MulticrewCore> MulticrewCore::multicrewCore() {
	if( ::multicrewCore==0 ) ::multicrewCore = new MulticrewCore();

	// dout << "MulticrewCore::multicrewCore()" << std::endl;
	return ::multicrewCore;
}


struct MulticrewCore::Data {	
	/* mode handling */
	Mode mode;

	/* modules */
	std::map<std::string, MulticrewModule*> modules;
	SmartPtr<PositionModule> posModule;
	SmartPtr<FsuipcModule> fsuipcModule;

	/* network stuff */
	std::set<AsyncCallee*> asyncCallees;

	/* timing */
	__int64 perfTimerFreq;
	__int64 startTime;

	CRITICAL_SECTION critSect;
};


MulticrewCore::MulticrewCore() {
	d = new Data;
	d->mode = IdleMode;
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
	dout << "MulticrewCore" << std::endl;
	InitializeCriticalSection( &d->critSect );

	/* initialize timing stuff */
	if( !QueryPerformanceFrequency((LARGE_INTEGER*)&d->perfTimerFreq) )
		dlog << "No performance timer available" << std::endl;
	QueryPerformanceCounter( (LARGE_INTEGER*)&d->startTime );
}


void MulticrewCore::start() {
	/* create general modules */
	d->posModule = new PositionModule();
	d->fsuipcModule = new FsuipcModule();
}


MulticrewCore::~MulticrewCore() {
	dout << "~MulticrewCore" << std::endl;
	d->modules.clear();
	::multicrewCore = 0;
	CoUninitialize();

	DeleteCriticalSection( &d->critSect );
	delete d;
}


double MulticrewCore::time() {
	__int64 now;
	QueryPerformanceCounter( (LARGE_INTEGER*)&now );
	return ((double)now-d->startTime)/d->perfTimerFreq;
}


void MulticrewCore::registerModule( MulticrewModule *module ) {
	dout << "module " << module->moduleName() << " registered" << std::endl;

	// register module
	d->modules[module->moduleName().c_str()] = module;	

	// setup fsuipc watches
	if( !d->fsuipcModule.isNull() ) {
		SmartPtr<FileConfig> config = module->config();
		for( int i=1; ; i++ ) {
			std::ostringstream num;
			num << i;
			
			// get fsuipc line
			std::string line = config->stringValue( "fsuipc", num.str(), "" );
			if( line.length()==0 ) break;
			
			// break line into components
			std::vector<std::string> tokens =
               split( line, "," );
			if( tokens.size()!=3 )
				dlog << "invalid fsuipc for " << module->moduleName 
					 << " id " << num << std::endl;
			
			// convert into datatypes		
			int id = (int)to_uint( trim(tokens[0]) );
			int len = (int)to_uint( trim(tokens[1]) );
			bool safe = to_bool( trim(tokens[2]) );
			dout << "fsuipc watch " << id << " len=" << len 
				 << " safe=" << safe << std::endl;
			
			// add to watches
			d->fsuipcModule->watch( id, len, safe );
		}
	}
}


void MulticrewCore::unregisterModule( MulticrewModule *module ) {
	dout << "module " << module->moduleName() << " unregistered" << std::endl;

	// find and remove module from module list
	std::map<std::string, MulticrewModule*>::iterator res = d->modules.find( module->moduleName().c_str() );
	if( res!=d->modules.end() ) {
		d->modules.erase( res );
	}
}


MulticrewCore::Mode MulticrewCore::mode() {
	return d->mode;
}


void MulticrewCore::setMode( Mode newMode ) {
	if( newMode!=d->mode ) {
		d->mode = newMode;
		modeChanged.emit( d->mode );
	}
}


void MulticrewCore::log( std::string line ) {
	logged.emit( line.c_str() );
}


void MulticrewCore::prepare( SmartPtr<Connection> con ) {
	// finally start fsuipc and position module
	if( d->posModule.isNull() ) start();

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


void MulticrewCore::triggerAsyncCallback( AsyncCallee *callee ) {
	EnterCriticalSection( &d->critSect );
	bool first = d->asyncCallees.size()==0;

	// add callee to list of callback requesters
	d->asyncCallees.insert( callee );

	// let async callback handler know about the new callback
	if( first ) initAsyncCallback.emit();
	LeaveCriticalSection( &d->critSect );
}


void MulticrewCore::callbackAsync() {
	EnterCriticalSection( &d->critSect );
	for( std::set<AsyncCallee*>::iterator it = d->asyncCallees.begin();
		 it!=d->asyncCallees.end();
		 it++ ) {
		// do callback
		(*it)->asyncCallback();
	}
	d->asyncCallees.clear();
	LeaveCriticalSection( &d->critSect );
}



/*********************************************************************************/


void AsyncCallee::triggerAsyncCallback() {
	MulticrewCore::multicrewCore()->triggerAsyncCallback( this );
}
