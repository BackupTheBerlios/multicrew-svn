/*
Multicrew
Copyright (C) 2004 Stefan Schimanski

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

#include <iostream>
#include <string>
#include <windows.h>

HANDLE dest;

BOOL CALLBACK handleLanguage( HANDLE hModule, LPCTSTR lpszType, LPCTSTR lpszName, 
							  WORD wIDLanguage, LONG_PTR lParam ) {
	std::cout << "Copying \"" << (void*)lpszName << "\" of type " << (void*)lpszType << std::endl;
	HRSRC res = FindResourceEx( (HMODULE)hModule, lpszType, lpszName, wIDLanguage );
	if( res==NULL ) {
		std::cerr << "Can't find resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	HGLOBAL global = LoadResource( (HMODULE)hModule, res );
	if( global==NULL ) {
		std::cerr << "Can't load resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	LPVOID data = LockResource( global );
	if( data==NULL ) {
		std::cerr << "Can't lock resource \"" << lpszName << "\"" << std::endl; 
		return FALSE;
	}

	return UpdateResource( 
		dest, 
		lpszType, 
		lpszName, 
		wIDLanguage, 
		data, 
		SizeofResource((HMODULE)hModule, res) );	
}

BOOL CALLBACK handleName( HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam ) {
	return EnumResourceLanguages( (HMODULE)hModule, lpszType, lpszName, 
								  (ENUMRESLANGPROCA)handleLanguage, lParam );
}

BOOL CALLBACK handleType( HMODULE hModule, LPTSTR lpszType, LONG_PTR lParam ) {
	return EnumResourceNames( hModule, lpszType, handleName, lParam );
}

int main(int argc, char* argv[])
{
	if( CopyFile(argv[2], argv[3], FALSE)==FALSE ) {
		std::cerr << "Can't copy \"" << argv[2] << "\" to \"" << argv[3] << "\"" << std::endl;
		std::cerr << GetLastError() << std::endl;
		return -1;
	}
	
	HMODULE source = LoadLibraryEx( argv[1], NULL, 0 ); //, LOAD_LIBRARY_AS_DATAFILE ); 
	if( source==NULL ) {
		std::cerr << "Can't open externalize library \"" << argv[1] << "\"" << std::endl;
		std::cerr << GetLastError() << std::endl;
		return -1;
	}	

	dest = BeginUpdateResource( argv[3], FALSE );
	if( dest==NULL ) {
		std::cerr << "Can't open destination library \"" << argv[3] << "\"" << std::endl;
		std::cerr << GetLastError() << std::endl;
		return -1;
	}

	if( EnumResourceTypes( source, handleType, 0 )==FALSE ) {
		std::cerr << "Couldn't enumerate all resources in library \"" << argv[3] << "\"" << std::endl;
		std::cerr << GetLastError() << std::endl;
		return -1;
	}

	if( EndUpdateResource( dest, FALSE )==FALSE ) {
		std::cerr << "Couldn't add resources to library \"" << argv[3] << "\"" << std::endl;
		std::cerr << GetLastError() << std::endl;
		return -1;
	}
	return 0;
}

