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

#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include "common.h"

// copyright 1999, James M. Curran
#include <strstream>
#include <ostream>
#include <windows.h>

#include "debug.h"
#include "multicrewcore.h"

using std::ostream;
class LogStream : public ostream {
private:
	class LogStreamBuf : public std::strstreambuf {
	public:
		LogStreamBuf() : core( MulticrewCore::multicrewCore() ) {}
	protected:
		virtual int sync() {
			sputc('\0');
			if( strlen(str())>0 ) {				
				OutputDebugString( str() );
				core->log(str());
			}

			freeze(false);
			setp(pbase(), pbase(), epptr());
			return 0;
		}

		SmartPtr<MulticrewCore> core;
	};
	
	LogStreamBuf m_buf;

public:
	LogStream() : ostream(&m_buf) {}
	~LogStream() { m_buf.pubsync();}
};

extern DLLEXPORT LogStream log;

#endif