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
#include <windows.h>

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
	GaugePrepare prepare( "multicrew\\multicrewgauge.dll", "gauges" );
	if( !prepare.prepare( "multicrewh", gauge ) ) {
		dlog << "Preparation of \""
			 << "gauges\\multicrewh"+gauge
			 << "\" failed." << std::endl;
		return false;
	}

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

	if( argc!=2 ) {
		dlog << "Syntax: multicrewprepare PMDG_737NG_Main.gau" << std::endl;
		return 1;
	}

	if( prepare.prepare( argv[1] ) )
		return 0;
	else 
		return 1;
}

