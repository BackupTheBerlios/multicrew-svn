/*
Multicrew
Copyright (C) 2004 Stefan Schimanski

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

#include <iostream>
#include <string>
#include <set>
#include <deque>
#include <vector>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../stlplus/source/string_utilities.hpp"
#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/log.h"
#include "gaugeprepare.h"


class CommandLinePrepare {
public:
	CommandLinePrepare();

	bool prepare( std::string gauge );

private:
	virtual void logged( const char *line ) {
		std::cerr << line;
	}

	Slot1<CommandLinePrepare, const char*> loggedSlot;
	SmartPtr<MulticrewCore> core;
};


CommandLinePrepare::CommandLinePrepare() 
	: core( MulticrewCore::multicrewCore() ), 
	  loggedSlot( 0, this, CommandLinePrepare::logged ) {
	loggedSlot.connect( &core->logged );
}


bool CommandLinePrepare::prepare( std::string gauge ) {
	// up to date check
	std::string target = "gauges\\multicrewh"+gauge;
	struct _stat targetStat;
	int result;
	result = _stat( target.c_str(), &targetStat );
	if( result==0 ) {
		struct _stat sourceStat;
		result = _stat( "multicrew\\multicrewgauge.dll", &sourceStat );
		if( result!=0 ) {
			dlog << "Can't stat \"multicrew\\multicrewgauge.dll\"" << std::endl;
			return false;
		}
		
		if( targetStat.st_mtime>=sourceStat.st_mtime )
			return true; // target is newer than source -> skip
	}

	// prepare host gauge
	GaugePrepare prepare( "multicrew\\multicrewgauge.dll", "gauges" );
	if( !prepare.prepare( "multicrewh", gauge ) ) {
		dlog << "Preparation of \""
			 << "gauges\\multicrewh"+gauge
			 << "\" failed." << std::endl;
		return false;
	}

	// copy client gauge
	std::string from = "gauges\\multicrewh"+gauge;
	std::string to = "gauges\\multicrewc"+gauge;
	int ret = CopyFile( from.c_str(), to.c_str(), FALSE );
	if( ret==FALSE ) {
		dlog << "Can't copy \"" << from << "\" to \"" << to << "\""
			 << " (" << GetLastError() << ")" << std::endl;
		return false;
	}
	
	return true;
}


int main( int argc, char* argv[] ) {
	CommandLinePrepare prepare;

	// syntax
	if( argc!=2 ) {
		dlog << "Syntax: multicrewpanelprepare aircraft\\PMDG737-700\\panel\\panel.cfg" << std::endl;
		return 1;
	}

	// get gauge dll names
	FileConfig cfg( argv[1] );
	std::set<std::string> dlls;
	for( unsigned window=0;; window++ ) {
		// search in [Window Titles]
		std::string windowKey = "Window" 
			+ to_string( window, 10, radix_c_style_or_hash, 2 );
		std::string value = cfg.stringValue( "Window Titles", windowKey, "" );
		if( value.length()==0 ) break;

		// get gauges
		for( unsigned gauge=0;; gauge++ ) {
			// look in "WindowXY"
			std::string gaugeKey = "gauge" 
				+ to_string( gauge, 10, radix_c_style_or_hash, 2 );
			value = cfg.stringValue( windowKey, gaugeKey, "" );
			if( value.length()==0 ) break;
			
			// get gauge dll name from:
			// PMDG_737NG_Main!PanelSwitcher, 473,593,256,26
			std::vector<std::string> parts = split( value, "!" );
			if( parts.size()>0 ) {
				dlls.insert( parts[0] );
			}
		}
	}

	// prepare gauges
	std::set<std::string>::iterator it = dlls.begin();
	while( dlls.end()!=it ) {
		dlog << "Handling \"" << *it << "\"..." << std::endl;

		if( !prepare.prepare( *it + ".gau" ) )
			dlog << "Failed" << std::endl;

		it++;
	}
	
	return 0;
}

