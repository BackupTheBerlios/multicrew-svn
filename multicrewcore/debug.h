#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "common.h"

// copyright 1999, James M. Curran
#include <strstream>
#include <ostream>
#include <windows.h>

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
  { m_buf.pubsync();}

 };

#else // defined (_DEBUG)

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

extern DLLEXPORT DebugStream dout;

#endif
