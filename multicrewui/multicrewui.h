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


#ifndef MULTICREWUI_H_INCLUDED
#define MULTICREWUI_H_INCLUDED

#include "common.h"
#include "resource.h"
#include "../multicrewcore/thread.h"
#include "../multicrewcore/multicrewcore.h"

class MulticrewUI : private Thread {
public:
	MulticrewUI( HWND hwnd );
	virtual ~MulticrewUI();	

	void host();
	void connect();
	void disconnect();
	void status();
	void start();
	void log();
	void async();

	HMENU newMenu();

private:
	void updateMenu();
	void disconnected();
	void logged( const char *line );
	void asyncSlot();
	void modeChanged( MulticrewCore::Mode newMode );
	virtual unsigned threadProc( void *param );

	struct Data;
	friend Data;
	Data *d;
};


#endif
