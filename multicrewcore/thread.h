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

#ifndef MULTICREWCORE_THREAD_H_INCLUDED
#define MULTICREWCORE_THREAD_H_INCLUDED

#include "common.h"

class DLLEXPORT Thread {
 public:
	Thread();
	virtual ~Thread();

	void startThread( void *param );
	unsigned stopThread();
	bool setPriority( int priority );
	bool postThreadMessage( UINT Msg, WPARAM wParam, LPARAM lParam );

 protected:
	virtual unsigned threadProc( void *param )=0;
	bool shouldExit( unsigned wait=0 );

 private:
	DWORD threadProcImpl( LPVOID param );

	struct Data;
	friend Data;
	Data *d;
};


#endif
