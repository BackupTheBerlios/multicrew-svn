/*
Multicrew
Copyright (C) 2005 Stefan Schimanski

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

#include <windows.h>

#include "callback.h"

#pragma pack(push,1)
struct CallbackThunk
{
	BYTE    mov;          // mov edx, pThis
	DWORD   thisPtr;      //
	BYTE    jmp;          // jmp WndProc
	DWORD   relProc;      // relative jmp	
};
#pragma pack(pop)

struct CallbackAdapterBase::Data {
	CallbackThunk thunk;
};

CallbackAdapterBase::CallbackAdapterBase( void *methodThunk ) {
	d = new Data;
	d->thunk.mov = 0xba;
	d->thunk.thisPtr = PtrToUlong( this );
	d->thunk.jmp = 0xe9;		
	d->thunk.relProc = (DWORD)(methodThunk) - (DWORD)(&d->thunk) - sizeof(CallbackThunk);	

	VirtualProtect( &d->thunk, sizeof(CallbackThunk), PAGE_EXECUTE_READWRITE, NULL );
	FlushInstructionCache( GetCurrentProcess(), &d->thunk, sizeof(CallbackThunk) );
}

CallbackAdapterBase::~CallbackAdapterBase() {
	delete d;
}

void *CallbackAdapterBase::callback() {
	return &d->thunk;
}
