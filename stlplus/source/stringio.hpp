#ifndef STRINGIO_HPP
#define STRINGIO_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Classes for redirecting I/O to/from a string

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include "textio.hpp"
#include <string>

////////////////////////////////////////////////////////////////////////////////
// string Output

class ostext : public otext
{
public:
  ostext(void);
  std::string& get_string(void);
  const std::string& get_string(void) const;
};

class osbuff : public obuff
{
protected:
  friend class ostext;
  std::string str;
public:
  osbuff(void);
protected:
  virtual unsigned put (unsigned char);
};

////////////////////////////////////////////////////////////////////////////////
// string Input

class istext : public itext
{
public:
  istext(const std::string& data);
  std::string& get_string(void);
  const std::string& get_string(void) const;
};

class isbuff : public ibuff
{
protected:
  friend class istext;
  std::string data;
  unsigned i;
public:
  isbuff(const std::string& data);
protected:
  virtual int peek (void);
  virtual int get (void);
};

////////////////////////////////////////////////////////////////////////////////
#endif
