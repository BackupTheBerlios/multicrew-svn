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

#include "gaugeprepare.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/log.h"


struct GaugePrepare::Data {
	Data( GaugePrepare *gp ) 
		: handleLanguageAdapter( gp, GaugePrepare::handleLanguage ),
		  handleNameAdapter( gp, GaugePrepare::handleName ),
		  handleTypeAdapter( gp, GaugePrepare::handleType ) {
	}

	std::string multicrewgaugeDll;
	std::string moduleDir;
	HANDLE dest;

	CallbackAdapter5<BOOL, GaugePrepare, HMODULE, LPCTSTR, 
					 LPCTSTR, WORD, LONG_PTR> handleLanguageAdapter;
	CallbackAdapter4<BOOL, GaugePrepare, HMODULE, 
					 LPCTSTR, LPTSTR, LONG_PTR> handleNameAdapter;
	CallbackAdapter3<BOOL, GaugePrepare, HMODULE, 
					 LPTSTR, LONG_PTR> handleTypeAdapter;
};


GaugePrepare::GaugePrepare( std::string multicrewgaugeDll, 
							std::string moduleDir ) {
	d = new Data( this );
	d->multicrewgaugeDll = multicrewgaugeDll;
	d->moduleDir = moduleDir;
}


GaugePrepare::~GaugePrepare() {
	delete d;
}
	

BOOL GaugePrepare::handleLanguage( HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, 
							  WORD wIDLanguage, LONG_PTR lParam ) {
	dout << "Copying \"" << (void*)lpszName 
		 << "\" of type " << (void*)lpszType << std::endl;
	HRSRC res = FindResourceEx( hModule, lpszType, lpszName, wIDLanguage );
	if( res==NULL ) {
		dlog << "Can't find resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	HGLOBAL global = LoadResource( hModule, res );
	if( global==NULL ) {
		dlog << "Can't load resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	LPVOID data = LockResource( global );
	if( data==NULL ) {
		dlog << "Can't lock resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	return UpdateResource( 
		d->dest, 
		lpszType, 
		lpszName, 
		wIDLanguage, 
		data, 
		SizeofResource( hModule, res) );	
}

BOOL GaugePrepare::handleName( HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam ) {
	return EnumResourceLanguages( (HMODULE)hModule, lpszType, lpszName, 
								  /*(ENUMRESLANGPROCA)*/d->handleLanguageAdapter.callback(), 
								  lParam );
}

BOOL GaugePrepare::handleType( HMODULE hModule, LPTSTR lpszType, LONG_PTR lParam ) {
	return EnumResourceNames( hModule, lpszType, 
							  d->handleNameAdapter.callback(), lParam );
}


bool GaugePrepare::prepare( std::string prefix, std::string gauge ) {
	std::string target = d->moduleDir+"\\"+prefix+gauge;

	int ret = CopyFile( d->multicrewgaugeDll.c_str(), target.c_str(), FALSE);
	if( ret==FALSE ) {
		dlog << "Can't copy \"" << d->multicrewgaugeDll 
			 << "\" to \"" << target << "\""
			 << " (" << GetLastError() << ")" << std::endl;
		return false;
	}
	
	std::string source = d->moduleDir+"\\"+gauge;
	HMODULE hsource = LoadLibraryEx( source.c_str(), NULL, 0 ); //, LOAD_LIBRARY_AS_DATAFILE ); 
	if( hsource==NULL ) {
		dlog << "Can't open externalize library \"" 
			 << source << "\"" << " (" << GetLastError() << ")" << std::endl;
		return false;
	}	

	d->dest = BeginUpdateResource( target.c_str(), FALSE );
	if( d->dest==NULL ) {
		dlog << "Can't open destination library \"" 
			 << target << "\""
			 << " (" << GetLastError() << ")" << std::endl;
		return false;
	}

	if( EnumResourceTypes( hsource, d->handleTypeAdapter.callback(), 0 )==FALSE ) {
		dlog << "Couldn't enumerate all resources in library \"" 
			 << source << "\"" << " (" << GetLastError() << ")" << std::endl;
		return false;
	}

	if( EndUpdateResource( d->dest, FALSE )==FALSE ) {
		dlog << "Couldn't add resources to library \"" 
			 << target << "\"" << " (" << GetLastError() << ")" << std::endl;
		return false;
	}

	return true;
}


