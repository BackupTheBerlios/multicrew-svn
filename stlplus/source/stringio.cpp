/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/

#include "os_fixes.hpp"
#include "stringio.hpp"

////////////////////////////////////////////////////////////////////////////////

ostext::ostext(void) : otext(new osbuff)
{
}

std::string& ostext::get_string(void)
{
  return dynamic_cast<osbuff*>(m_buffer.pointer())->str;
}

const std::string& ostext::get_string(void) const
{
  return dynamic_cast<const osbuff*>(m_buffer.pointer())->str;
}

////////////////////////////////////////////////////////////////////////////////

osbuff::osbuff(void)
{
}

unsigned osbuff::put(unsigned char ch)
{
  str += (char)ch;
  return 1;
}

////////////////////////////////////////////////////////////////////////////////

istext::istext(const std::string& data) : itext(new isbuff(data))
{
}

std::string& istext::get_string(void)
{
  return dynamic_cast<isbuff*>(m_buffer.pointer())->data;
}

const std::string& istext::get_string(void) const
{
  return dynamic_cast<const isbuff*>(m_buffer.pointer())->data;
}

////////////////////////////////////////////////////////////////////////////////

isbuff::isbuff(const std::string& d) : data(d), i(0)
{
}

int isbuff::peek(void)
{
  return i >= data.size() ? -1 : (int)(unsigned char)data[i];
}

int isbuff::get(void)
{
  return i >= data.size() ? -1 : (int)(unsigned char)data[i++];
}

////////////////////////////////////////////////////////////////////////////////
