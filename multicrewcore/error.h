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

#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#include "common.h"

// copyright 1999, James M. Curran
#include <strstream>
#include <ostream>
#include <windows.h>
#include <string>

DLLEXPORT std::string formatError( HRESULT hr );
#define fe formatError

#if defined (_DEBUG)
 using std::ostream;
 class ErrorStream : public ostream
 {
 private:
  class DebugStreamBuf : public std::strstreambuf
  {
  protected:
   virtual int sync()
   {
    sputc('\0');
	if( strlen(str())>0 ) {
		MessageBox( 0, str(), TEXT("Multicrew Error"), MB_OK );   
		OutputDebugStringA(str());
	}

    freeze(false);
    setp(pbase(), pbase(), epptr());
    return 0;
   }
  };

  DebugStreamBuf m_buf;

 public:
  ErrorStream() : ostream(&m_buf)
  {}

  ~ErrorStream()
  { m_buf.pubsync();}

 };

#else // defined (_DEBUG)

 class ErrorStream
 {
 public:
  template <typename T>
  inline const ErrorStream& operator<<(T) const
    {return(*this);}

  typedef std::basic_ostream<char>& (__cdecl *
endl_type)(std::basic_ostream<char>&);

  inline const ErrorStream& operator<<(const endl_type T) const
    {return(*this);}
 } ;

#endif

extern DLLEXPORT ErrorStream derr;

#endif
