/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

------------------------------------------------------------------------------*/
#include "persistent_exceptions.hpp"
#include "string_utilities.hpp"

////////////////////////////////////////////////////////////////////////////////
// exceptions

persistent_illegal_type::persistent_illegal_type(const std::string& type) throw() : 
  std::logic_error(std::string("illegal type: ") + type)
{
}

persistent_illegal_type::persistent_illegal_type(unsigned short key) throw() : 
  std::logic_error(std::string("illegal key: ") + to_string(key))
{
}

persistent_illegal_type::~persistent_illegal_type(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////

persistent_dump_failed::persistent_dump_failed(const std::string& message) throw() :
  std::runtime_error(std::string("dump failed: ") + message)
{
}

persistent_dump_failed::~persistent_dump_failed(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////

persistent_restore_failed::persistent_restore_failed(const std::string& message) throw() :
  std::runtime_error(std::string("restore failed: ") + message)
{
}

persistent_restore_failed::~persistent_restore_failed(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////
