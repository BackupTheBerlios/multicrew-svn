/*
 *  Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>

#include "config.h"
#include "artconf.h"


struct FileConfig::Data {
    conf_pstruct config;
};

FileConfig::FileConfig( const std::string &file ) {
    d = new Data;
    d->config = conf_init( file.c_str() );
}


FileConfig::~FileConfig() {
    delete d;
}


int FileConfig::intValue( const std::string &group, const std::string &key, int def ) {
    conf_togroup( group.c_str(), d->config );

    char *kk;
    if ((kk = conf_getany(key.c_str(), d->config)))
        return atol(kk);
    else
        return def;
}


std::string FileConfig::stringValue( const std::string &group, const std::string &key, 
									 const std::string &def ) {
    conf_togroup( group.c_str(), d->config );
	
    char *kk;
    if ((kk = conf_getany(key.c_str(), d->config)))
        return kk;
    else
        return def;
}


bool FileConfig::boolValue( const std::string &group, const std::string &key, bool def ) {
    conf_togroup( group.c_str(), d->config );
	
    char *kk;
    if ((kk = conf_getany(key.c_str(), d->config))) {
		std::string val = kk;
		// dout << "FileConfig::getBool( " << group << ", " << key << ", " << def << " ) = \"" << val << "\"" << endl;
		
		// TODO: find better way (e.g. caseless compare etc.)
		if( val=="true" || val=="True" || val=="TRUE" || val=="yes" || val=="Yes" || val=="YES" || val=="1" )
			return true;
		else
			return false;
    } else
        return def;
}


void FileConfig::setNumber( const std::string &group, const std::string &key, int value ) {
	conf_newgroup( group.c_str(), d->config );
	conf_togroup( group.c_str(), d->config );
	conf_newnumber( key.c_str(), value, d->config );
}


void FileConfig::setString( const std::string &group, const std::string &key, 
							const std::string &value ) {
	conf_newgroup( group.c_str(), d->config );
	conf_togroup( group.c_str(), d->config );
	conf_newstring( key.c_str(), value.c_str(), d->config );
}


void FileConfig::setBool( const std::string &group, const std::string &key, bool value ) {
	conf_newgroup( group.c_str(), d->config );
	conf_togroup( group.c_str(), d->config );
	conf_newnumber( key.c_str(), value?1:0, d->config );
}


int FileConfig::sync() {
    return conf_savetodisk( d->config );
}
