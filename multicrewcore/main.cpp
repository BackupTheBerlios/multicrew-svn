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

#ifdef _DEBUG
#include <CRTDBG.H>
#endif

#include "multicrewcore.h"
#include "streams.h"
#include "log.h"
#include "callback.h" 
#include "shared.h" 


std::string formatError( HRESULT hr ) {
	char *buf;
	FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &buf,
        0, NULL );
	char ret[1024];
	sprintf( ret, "%s 0x%x", (int)hr );
	if( buf==0 ) {
		sprintf( ret, "0x%x", (int)hr );
	} else {
		sprintf( ret, "%s (0x%x)", buf, (int)hr );
	}
	return ret;
}


#undef dout
#undef dlog
#undef derr
DLLEXPORT DebugStream dout;
DLLEXPORT ErrorStream derr;
DLLEXPORT LogStream dlog;
DLLEXPORT CRITICAL_SECTION ostreamCritSec;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		OutputDebugString( "core attach\n" );
		InitializeCriticalSection( &ostreamCritSec );

#ifdef _DEBUG
        // Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

        // Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

        // Turn off CRT block checking bit
		tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;

        // Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
#endif
	} break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DeleteCriticalSection( &ostreamCritSec );
		OutputDebugString( "core detach" );
		break;
	}
    return TRUE;
}
