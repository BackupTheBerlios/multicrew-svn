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

#include "multicrewcore.h"
#include "streams.h"
#include "shared.h" 

bool inited = false;
static CRITICAL_SECTION gSharedCritSect;

int Shared::ref() {
#ifdef SHARED_DEBUG 
	dout << (void*)this << ":" << typeid(*this).name() 
		 << "::ref() = " << refcount+1 << std::endl;
#endif
	if( !inited ) { 
		inited=true; 
		InitializeCriticalSection( &gSharedCritSect ); 
	}

	EnterCriticalSection( &gSharedCritSect );
	refcount++;
	LeaveCriticalSection( &gSharedCritSect );
	return refcount;
} 

int Shared::deref() {
	if( !inited ) InitializeCriticalSection( &gSharedCritSect );
	EnterCriticalSection( &gSharedCritSect );
	refcount--;
	int ret = refcount;
#ifdef SHARED_DEBUG
	dout << (void*)this << ":" << typeid(*this).name() 
		 << "::deref() = " << refcount << std::endl;
#endif
	if( ret==0 ) {
#ifdef SHARED_DEBUG
		dout << (void*)this << ":" << typeid(*this).name() 
			 << " deleting itself" << std::endl;
#endif
		LeaveCriticalSection( &gSharedCritSect );
		delete this; 
	} else
		LeaveCriticalSection( &gSharedCritSect );
	
	return ret;
}
