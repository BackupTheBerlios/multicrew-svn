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

#include <windows.h>
#include "multicrewcore.h"
#include "debug.h"
#include "callback.h"

// TODO: make thread safe!!!!
void *CallbackAdapterBase::adapter;
static DWORD gTlsIndex; 

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		dout << "core attach" << std::endl;
		
		// index for local thread memory
		gTlsIndex = TlsAlloc();
		if( gTlsIndex==TLS_OUT_OF_INDEXES ) return FALSE; 
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		// release local thread memory
		TlsFree( gTlsIndex );
		
		dout << "core detach" << std::endl;
		break;
	}
    return TRUE;
}
