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

#ifndef MULTICREWCORE_CONFIG_H_INCLUDED
#define MULTICREWCORE_CONFIG_H_INCLUDED

#include "common.h"

#include <string>

#include "shared.h"


class Config {
 public:
	virtual bool boolValue( const std::string &category, const std::string &key, bool def )=0;
	virtual std::string stringValue( const std::string &category, const std::string &key, const std::string &def )=0;
	virtual int intValue( const std::string &category, const std::string &key, int def )=0;
};


class DLLEXPORT RegistryConfig : public Config {
 public:
	static Config *config();

	bool boolValue( const std::string &category, const std::string &key, bool def );
	std::string stringValue( const std::string &category, const std::string &key, const std::string &def );
	int intValue( const std::string &category, const std::string &key, int def );

 private:
	RegistryConfig();
};


class DLLEXPORT FileConfig : public Shared, public Config {
 public:
    FileConfig( const std::string &file );
    virtual ~FileConfig();
	
    int sync();
	
    int intValue( const std::string &group, const std::string &key, int def );
    bool boolValue( const std::string &group, const std::string &key, bool def );
    std::string stringValue( const std::string &group, const std::string &key, const std::string &def=std::string() );

    void setNumber( const std::string &group, const std::string &key, int value );
    void setBool( const std::string &group, const std::string &key, bool value );
    void setString( const std::string &group, const std::string &key, const std::string &value );

 private:
	struct Data;
    Data* d;
	friend Data;
};


#endif
