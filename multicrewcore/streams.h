#ifndef MULTICREWCORE_STREAMS_H_INCLUDED
#define MULTICREWCORE_STREAMS_H_INCLUDED

#include "common.h"

// parts copyright 1999, James M. Curran
#include <strstream>
#include <ostream>
#include <windows.h>

DLLEXPORT std::string formatError( HRESULT hr );
#define fe formatError

#if defined (_DEBUG)
using std::ostream;

 class DebugStream : public ostream
 {
 private:
  class DebugStreamBuf : public std::strstreambuf
  {
  protected:
   virtual int sync()
   {
    sputc('\0');
	HANDLE thread = GetCurrentThread();
	char s[16];
	sprintf( s, "%x: ", (int)thread );
	OutputDebugStringA(s);
    OutputDebugStringA(str());
	
    freeze(false);
    setp(pbase(), pbase(), epptr());
    return 0;
   }
  };

  DebugStreamBuf m_buf;

 public:
  DebugStream() : ostream(&m_buf)
  {}

  ~DebugStream()
	  {} // m_buf.pubsync();}

 };

#else

 class DebugStream
 {
 public:
  template <typename T>
  inline const DebugStream& operator<<(T) const
    {return(*this);}

  typedef std::basic_ostream<char>& (__cdecl *
endl_type)(std::basic_ostream<char>&);

  inline const DebugStream& operator<<(const endl_type T) const
    {return(*this);}
 } ;
#endif


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
	  {} // m_buf.pubsync();}

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
extern DLLEXPORT DebugStream dout;

#ifdef _DEBUG
extern DLLEXPORT CRITICAL_SECTION ostreamCritSec;

// And now some C magic to make ostreams thread aware (thanks to C guru Janko Heilgeist)
#define dout do { EnterCriticalSection( &ostreamCritSec ); dout
#define derr do { EnterCriticalSection( &ostreamCritSec ); derr
#define endl endl; LeaveCriticalSection( &ostreamCritSec ); } while(false)
#endif

#endif
