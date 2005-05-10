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
#include <string>
#include <deque>
#include <map>

#include "gauges.h"
#include "multicrewgauge.h"
#include "apihijack.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/config.h"
#include "../multicrewcore/log.h"
#include "../multicrewcore/shared.h"
#include "../multicrewcore/multicrewcore.h"


GAUGESLINKAGE Linkage = {														
	0x00000013,											
	0,
	0,
	0,													
	0,													
	FS9LINK_VERSION, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

GAUGESIMPORT ImportTable;


/*******************************************************************/
class ModuleLoader : public Shared {
public:
	ModuleLoader( GAUGESLINKAGE *gaugeLinkage, std::string name );
	virtual ~ModuleLoader();

	GAUGESLINKAGE *linkage() {
		return gaugeLinkage;
	}

	VoidCallbackAdapter<ModuleLoader> moduleInitCallback;
	VoidCallbackAdapter<ModuleLoader> moduleDeinitCallback;

private:
	void module_init();
	void module_deinit();
	
	GAUGESLINKAGE *gaugeLinkage;
	SmartPtr<GaugeModule> mg;
	void (__stdcall *oldModuleInit)();
	void (__stdcall *oldModuleDeinit)();
	std::deque<PGAUGE_CALLBACK> oldCallbacks;
	std::string name;
};


ModuleLoader::ModuleLoader( GAUGESLINKAGE *gaugeLinkage, std::string name ) 
 : moduleInitCallback( this, ModuleLoader::module_init ),
   moduleDeinitCallback( this, ModuleLoader::module_deinit ),
   gaugeLinkage( gaugeLinkage ),
   name( name )
{
	// setup linkage
	oldModuleInit = gaugeLinkage->ModuleInit;
	oldModuleDeinit = gaugeLinkage->ModuleDeinit;
	gaugeLinkage->ModuleInit = moduleInitCallback.callback();
	gaugeLinkage->ModuleDeinit = moduleDeinitCallback.callback();
}


ModuleLoader::~ModuleLoader() {
	gaugeLinkage->ModuleInit = oldModuleInit;
	gaugeLinkage->ModuleDeinit = oldModuleDeinit;
	delete mg;
}


void ModuleLoader::module_init() {
	dout << "module_init " << name <<std::endl; 

	// create gauge module
	mg = new GaugeModule( name );

	// call module init
	if( oldModuleInit!=NULL ) (*oldModuleInit)();

	// setup multicrew Linkage
	for( int i=0; gaugeLinkage->gauge_header_ptr[i]!=0 && i<255; i++ ) {

		if( gaugeLinkage->gauge_header_ptr[i]->gauge_callback==mg->installCallback() ) {
			dout << "Skipping " << gaugeLinkage->gauge_header_ptr[i]->gauge_name
				 << std::endl;
			oldCallbacks.push_back( (PGAUGE_CALLBACK)(int)gaugeLinkage->gauge_header_ptr[i]->user_area[9] );
        } else {
			dout << "Wrapping gauge " << gaugeLinkage->gauge_header_ptr[i]->gauge_name
				 << " at " << gaugeLinkage->gauge_header_ptr[i] 
				 << " callback " << gaugeLinkage->gauge_header_ptr[i]->gauge_callback
				 << " by " << (void*)mg->installCallback() << std::endl;

			// save callback
			oldCallbacks.push_back( gaugeLinkage->gauge_header_ptr[i]->gauge_callback );

			// install creation callback		
			gaugeLinkage->gauge_header_ptr[i]->user_area[9] = (FLOAT64)(int)(gaugeLinkage->gauge_header_ptr[i]->gauge_callback);
			gaugeLinkage->gauge_header_ptr[i]->gauge_callback = mg->installCallback();
		}
	}
}


void ModuleLoader::module_deinit() {
	dout << "module_deinit " << name <<std::endl; 
	mg = 0;

	// restore old callbacks, might be needed in deinit
	for( unsigned i=0; i<oldCallbacks.size(); i++ ) {
		gaugeLinkage->gauge_header_ptr[i]->gauge_callback = oldCallbacks[i];
	}
	oldCallbacks.clear();
	
	if( oldModuleDeinit!=NULL ) (*oldModuleDeinit)();
}


/************************ Registry *********************************/
CRITICAL_SECTION cs;
std::map<HMODULE,ModuleLoader*> gLoaders;


/******************** Hooking LoadLibraryA *************************/
__declspec(dllexport) HMODULE WINAPI MyLoadLibraryA( IN LPCSTR lpLibFileName );
typedef __declspec(dllexport) HMODULE WINAPI LoadLibraryType( IN LPCSTR lpLibFileName );
__declspec(dllexport) FARPROC WINAPI MyGetProcAddress( IN HMODULE hModule, IN LPCSTR lpProcName );
typedef __declspec(dllexport) FARPROC WINAPI GetProcAddressType( IN HMODULE hModule, IN LPCSTR lpProcName );
__declspec(dllexport) BOOL WINAPI MyFreeLibrary( IN HMODULE hModule );
typedef __declspec(dllexport) BOOL WINAPI FreeLibraryType( IN HMODULE hModule );


SDLLHook Kernel32Hook = 
{
    "KERNEL32.DLL",
    false, NULL,    // Default hook disabled, NULL function pointer.
    {
        { "LoadLibraryA", MyLoadLibraryA },
		{ "FreeLibrary", MyFreeLibrary },
        { NULL, NULL }
    }
};


HMODULE WINAPI MyLoadLibraryA( IN LPCSTR lpLibFileName ) {
	HMODULE ret = ((LoadLibraryType*)Kernel32Hook.Functions[0].OrigFn)(lpLibFileName);
	dout << "LoadLibraryA(\""
		 << lpLibFileName
		 << "\")=" 
		 << ret 
		 << std::endl;
	if( ret!=0 ) {
		EnterCriticalSection( &cs );
		std::string path( lpLibFileName );
		int nameLeft = path.rfind( "\\", -1 )+1;
		int nameRight = path.rfind( ".", path.length() );
		int lastDirLeft = path.substr( 0, nameLeft-1 ).rfind( "\\", -1 )+1;
		std::string lastDir = path.substr( lastDirLeft, nameLeft-lastDirLeft );
		OutputDebugString( "last dir " );
		OutputDebugString( lastDir.c_str() );
		OutputDebugString( "\n" );
		if( stricmp( lastDir.c_str(), "GAUGES\\" )==0 ) {
			// look for ModuleLoader
			std::map<HMODULE,ModuleLoader*>::iterator it = gLoaders.find(ret);
			if( it!=gLoaders.end() ) {
				// increase ref counter
				it->second->ref();
			} else {
				// get linkage of gauge module			
				GAUGESLINKAGE *gaugeLinkage = 
					(GAUGESLINKAGE *)GetProcAddress( ret, "Linkage" );
				
				if( gaugeLinkage!=0 ) {
					// get name of module
					std::string name = path.substr( nameLeft, nameRight-nameLeft );
					dlog << "Wrapping " << name << std::endl;
					
					// create loader object
					gLoaders[ret] = new ModuleLoader( gaugeLinkage, name );
					gLoaders[ret]->ref();
					OutputDebugString( "ModuleLoader created\n" );
				}
			}
		}
		LeaveCriticalSection( &cs );
	}

	return ret;
}


BOOL WINAPI MyFreeLibrary( IN HMODULE hModule ) {
	dout << "FreeLibrary(\""
		 << hModule
		 << "\")\n"
		 << std::endl;

	EnterCriticalSection( &cs );
	std::map<HMODULE,ModuleLoader*>::iterator it = gLoaders.find(hModule);
	if( it!=gLoaders.end() ) {
		// decrease ref counter and possibly delete entry in loader table
		int refCount = it->second->deref();
		if( refCount==0 ) gLoaders.erase( it );
	}
	LeaveCriticalSection( &cs );
	
	return ((FreeLibraryType*)Kernel32Hook.Functions[1].OrigFn)(hModule);	
}
		

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)	{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH: {
		OutputDebugString( "Loading MulticrewGauge\n" );
		
		InitializeCriticalSection( &cs );
		HookDll( &Kernel32Hook );
		
		OutputDebugString( "Loaded MulticrewGauge\n" );
	} break;
	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH: break;		
	case DLL_PROCESS_DETACH: {
		OutputDebugString( "Unloading MulticrewGauge\n" );
		gLoaders.clear();
		OutputDebugString( "Unloaded MulticrewGauge\n" );
	} break;
	}
    return TRUE;
}
