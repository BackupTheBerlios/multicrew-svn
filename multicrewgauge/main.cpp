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

#include "gauges.h"
#include "multicrewgauge.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/config.h"
#include "../multicrewcore/log.h"
#include "../multicrewcore/shared.h"
#include "../multicrewcore/multicrewcore.h"


// prototypes
void FSAPI	module_init(void);
void FSAPI	module_deinit(void);

// variables	
GAUGESIMPORT ImportTable =
{														
	{ 0x0000000F, (PPANELS)NULL },						
	{ 0x00000000, NULL }								
};													
															   
GAUGESLINKAGE	Linkage =								
	{														
			0x00000013,											
			module_init,										
			module_deinit,										
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

HMODULE gInstance = NULL;
static HMODULE gGaugeLibrary = NULL;
static GAUGESLINKAGE *gGaugeLinkage = NULL;
static GAUGESIMPORT *gGaugeImportTable = NULL;
static std::deque<PGAUGE_CALLBACK> gOldCallbacks;
static SmartPtr<MulticrewGauge> mg;


//////////////////////////////////////////////
// Code
//////////////////////////////////////////////

void FSAPI module_init( void ) {
	// debug
	dout << "gInstance=" << (int)gInstance << std::endl;
	dout << Linkage.ModuleID << std::endl;	
	
	// get gauge name that should be externalized 
	// (if the dll is called "multicrewcfoobar.gau", the externalized 
	// gauge is "foobar" in client mode, in the case of "multicrewhfoobar"
	// the host mode is used.
	TCHAR dllPathBuf[1024];
	GetModuleFileName( gInstance, dllPathBuf, 1024 );
	std::string dllPath( dllPathBuf );
	size_t filenameStart = dllPath.rfind( "\\", dllPath.length() )+1;
	std::string filename = dllPath.substr( filenameStart+10, dllPath.length()-filenameStart-10 );  // remove "...\multicrew"
	std::string path = dllPath.substr( 0, filenameStart );
	std::string prefix = dllPath.substr( filenameStart, 10 );
	std::string gaugeModule = filename.substr( 0, filename.length()-4 ); // remove ".gau"
	dout << "prefix=" << prefix << std::endl;

	// choose host or client mode
	if( prefix[9]=='h' )
		mg = new MulticrewGauge( true, gaugeModule );
	else
		mg = new MulticrewGauge( false, gaugeModule );
	bool ret = mg->init();
	if( !ret ) {
		dout << "Skip rest of initialization of \"" << prefix+filename << "\"" << std::endl;
		return;
	}

	dlog << "Wrapping gauge library \"" << gaugeModule << "\" in " << 
		(prefix[9]=='h'?"host mode":"client mode")
		<< std::endl;
	
	// load externalized gauge
	std::string gaugePathName( path + gaugeModule + ".gau" );
	gGaugeLibrary = LoadLibrary( gaugePathName.c_str() );
	if( gGaugeLibrary==NULL ) {
		derr << "Can't load gauge \"" << gaugePathName << "\"" << std::endl;
		return;
	}

	// get linkage
	gGaugeLinkage = (GAUGESLINKAGE*)GetProcAddress( gGaugeLibrary, "Linkage" );
	if( gGaugeLinkage==NULL ) {
		derr << "Can't get Linkage for \"" << path << "\"" << std::endl;
		return;
	}

	// setup gauge ImportTable
	gGaugeImportTable = (GAUGESIMPORT*)GetProcAddress( gGaugeLibrary, "ImportTable" );
	if( gGaugeImportTable==NULL ) {
		derr << "Can't get ImportTable for \"" << path << "\"" << std::endl;
		return;
	}
	memcpy( gGaugeImportTable, &ImportTable, sizeof(GAUGESIMPORT) );

	// call module init
	dlog << "Init gauge module \"" << gaugeModule << "\"" << std::endl; 
	if( gGaugeLinkage->ModuleInit!=NULL ) (*gGaugeLinkage->ModuleInit)();

	// setup multicrew Linkage
	for( int i=0; gGaugeLinkage->gauge_header_ptr[i]!=0 && i<255; i++ ) {
		dout << "Wrapping gauge \"" << gGaugeLinkage->gauge_header_ptr[i]->gauge_name << "\"" << std::endl;
		Linkage.gauge_header_ptr[i] = gGaugeLinkage->gauge_header_ptr[i];	

		// save callback
		gOldCallbacks.push_back( gGaugeLinkage->gauge_header_ptr[i]->gauge_callback );

		// install creation callback		
		gGaugeLinkage->gauge_header_ptr[i]->user_area[9] = (FLOAT64)(int)(gGaugeLinkage->gauge_header_ptr[i]->gauge_callback);
		gGaugeLinkage->gauge_header_ptr[i]->gauge_callback = mg->installCallback();		
	}

	// update ImportTable if changed
	memcpy( gGaugeImportTable, &ImportTable, sizeof(GAUGESIMPORT) );

	dlog << "Multicrew plane loaded" << std::endl;
}

void FSAPI module_deinit(void) {
	// restore old callbacks, might be needed in deinit
	mg = 0;

	for( unsigned i=0; i<gOldCallbacks.size(); i++ ) {
		gGaugeLinkage->gauge_header_ptr[i]->gauge_callback = gOldCallbacks[i];
	}

	dout << "ModuleDeinit" << std::endl;
	if( gGaugeLinkage!=NULL ) {
		if( gGaugeLinkage->ModuleDeinit!=NULL ) (*gGaugeLinkage->ModuleDeinit)();
		gGaugeLinkage = NULL;
	}

	dout << "FreeLibrary" << std::endl;
	if( gGaugeLibrary!=NULL ) {
		FreeLibrary( gGaugeLibrary );
		gGaugeLibrary = NULL;
	}
}						
		
BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)	{
	gInstance = hDLL;
	return TRUE;
}														
