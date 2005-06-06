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

#ifndef MULTICREWCORE_COMMON_H_INCLUDED
#define MULTICREWCORE_COMMON_H_INCLUDED

#ifdef MULTICREWCORE_EXPORTS // assume this is defined when we build the DLL
#define DLLEXPORT __declspec(dllexport)
#define DLLEXPORT_TEMPLATE
#else
#define DLLEXPORT __declspec(dllimport)
#define DLLEXPORT_TEMPLATE extern
#endif

#pragma warning (disable : 4355 4786)


#include "../stlplus/source/string_utilities.hpp"
#include "debug.h"


class DLLEXPORT AsyncCallee {
 public:
	void triggerAsyncCallback();
	virtual void asyncCallback()=0;
};


#endif
