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

#ifndef MULTICREWCORE_H_INCLUDED
#define MULTICREWCORE_H_INCLUDED

#include "common.h"

#include <string>

#include "shared.h"
#include "signals.h"
#include "network.h"


class DLLEXPORT MulticrewModule : public Shared, protected Receiver {
public:
	MulticrewModule( std::string moduleName, bool hostMode );
	virtual ~MulticrewModule();

	std::string moduleName();
	bool isHostMode();

protected:
	virtual void send( ModulePacket *packet, bool safe, bool sync );
	virtual std::string id();

private:
	friend class MulticrewCore;
	void connect( SmartPtr<Connection> con );
	void disconnect();

private:
	struct Data;
	Data *d;
};


class DLLEXPORT MulticrewCore : public Shared {
public:
	static SmartPtr<MulticrewCore> multicrewCore();	

	bool registerModule( MulticrewModule *module );
	void unregisterModule( MulticrewModule *module );

	bool isPlaneLoaded();
	bool isHostMode();
	Signal planeLoaded;
	Signal planeUnloaded;

	void prepare( SmartPtr<Connection> con );

	void log( std::string line );
	Signal1<const char *> logged;

private:
	MulticrewCore();
	virtual ~MulticrewCore();

	struct Data;
	Data *d;
};

#endif
