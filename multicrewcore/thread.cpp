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

#include <windows.h>

#include "thread.h"
#include "callback.h"
#include "log.h"


struct Thread::Data {
	Data( Thread *t )
		: threadProcAdapter( t, Thread::threadProcImpl ) {
	}

	HANDLE thread;
	HANDLE exitEvent;
	HANDLE exitedEvent;
	CallbackAdapter1<DWORD, Thread, LPVOID> threadProcAdapter;
};


Thread::Thread() {
	d = new Data( this );	
	d->thread = 0;
}


Thread::~Thread() {
	stopThread();
	delete d;
}


void Thread::startThread( void *param ) {
    // start thread
	if( d->thread==0 ) {
		d->exitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
		d->exitedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
		d->thread = CreateThread(
			NULL,
			0,
			d->threadProcAdapter.callback(),
			param,
			0,
			NULL );
		if( d->thread==NULL ) {
			CloseHandle( d->exitEvent );
			d->exitEvent = 0;
			dlog << "Cannot create thread" << std::endl;
		}
	}
}


unsigned Thread::stopThread() {
	if( d->thread==0 ) return 0;

	// stop thread
	if( d->thread!=0 ) { 
		SetEvent( d->exitEvent );
		WaitForSingleObject( d->exitedEvent, INFINITE );
		dout << "Thread has exited" << std::endl;
		DWORD ret;
		GetExitCodeThread( d->thread, &ret );
		CloseHandle( d->thread );
		CloseHandle( d->exitEvent );
		CloseHandle( d->exitedEvent );
		d->thread = 0;
		return ret;
	}

	return 0;
}


bool Thread::shouldExit( unsigned wait ) {
	// wait an amount of milliseconds or exit thread
	return WaitForSingleObject( d->exitEvent, wait )==WAIT_OBJECT_0;
}


DWORD Thread::threadProcImpl( LPVOID param ) {
	unsigned ret = threadProc( param );
	SetEvent( d->exitedEvent );
	return ret;
}


bool Thread::setPriority( int priority ) {
	return SetThreadPriority( d->thread, priority )==TRUE;
}
