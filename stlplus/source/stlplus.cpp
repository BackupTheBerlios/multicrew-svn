/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  ------------------------------------------------------------------------------*/

#include "stlplus.hpp"
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// work out the platform
// at present there are no variations between different Unix platforms so they all map onto the generic Unix platform

#undef PLATFORM
#ifdef _WIN32
#define PLATFORM string("Windows")
#endif
#ifndef PLATFORM
#define PLATFORM string("Generic Unix")
#endif

////////////////////////////////////////////////////////////////////////////////
// work out the compiler

#undef COMPILER
#ifdef __GNUC__
#define COMPILER (string("gcc v")+to_string(__GNUC__)+string(".")+to_string(__GNUC_MINOR__))
#endif
#ifdef _MSC_VER
#define COMPILER (string("MSVC v")+to_string(((float)_MSC_VER)/100.0))
#endif
#ifdef __BORLANDC__
#define COMPILER (string("Borland v")+to_string(__BORLANDC__/256)+string(".")+to_string(__BORLANDC__/16%16))
#endif

#ifndef COMPILER
#define COMPILER string("unknown compiler")
#endif

////////////////////////////////////////////////////////////////////////////////
// work out the kind of build
// there are two variants - debug and release

#undef VARIANT
#ifndef NDEBUG
#define VARIANT string("debug")
#else
#define VARIANT string("release")
#endif

////////////////////////////////////////////////////////////////////////////////
// report the platform-specific details of this build

std::string stlplus_build(void)
{
  return PLATFORM + ", " + COMPILER + ", " + VARIANT;
}

////////////////////////////////////////////////////////////////////////////////
