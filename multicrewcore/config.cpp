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

#include "streams.h"
#include "config.h"

static Config *config = 0;

Config *Config::config() {
	if( ::config==0 ) ::config = new Config();
	return ::config;
}

Config::Config() {
}

bool Config::boolValue( std::string category, std::string key, bool def ) {
	HKEY hkey;
	BYTE buf[256];
	DWORD size = 256;
	bool ret = def;
	DWORD type = REG_SZ;

	std::string cat = "Software\\Multicrew";
	if( category.length()>0 ) cat = cat + "\\" + category;
	if( RegOpenKey( HKEY_CURRENT_USER, cat.c_str(), &hkey )==ERROR_SUCCESS )
		if( RegQueryValueEx( hkey, key.c_str(), NULL, &type, buf, &size )==ERROR_SUCCESS )
			ret = (int)buf[0]!=0;
	RegCloseKey( hkey );
	return ret;
}

std::string Config::stringValue( std::string category, std::string key, std::string def ) {
	HKEY hkey;
	BYTE buf[256];
	DWORD size = 256;
	std::string ret = def;
	DWORD type = REG_SZ;

	std::string cat = "Software\\Multicrew";
	if( category.length()>0 ) cat = cat + "\\" + category;
	if( RegOpenKey( HKEY_CURRENT_USER, cat.c_str(), &hkey )==ERROR_SUCCESS )
		if( RegQueryValueEx( hkey, key.c_str(), NULL, &type, buf, &size )==ERROR_SUCCESS )
			ret = std::string((char*)buf);
	RegCloseKey( hkey );
	return ret;
}

int Config::intValue( std::string category, std::string key, int def ) {
	HKEY hkey;
	BYTE buf[256];
	DWORD size = 256;
	int ret = def;
	DWORD type = REG_SZ;

	std::string cat = "Software\\Multicrew";
	if( category.length()>0 ) cat = cat + "\\" + category;
	if( RegOpenKey( HKEY_CURRENT_USER, cat.c_str(), &hkey )==ERROR_SUCCESS )
		if( RegQueryValueEx( hkey, key.c_str(), NULL, &type, buf, &size )==ERROR_SUCCESS )
			ret = (int)buf[0];
	RegCloseKey( hkey );
	return ret;
}

