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

struct Packet;
struct ModulePacket;

class DLLEXPORT MulticrewModule : protected Receiver, public Sender {
public:
	MulticrewModule( std::string moduleName, bool hostMode, unsigned minSendWait=-1 );
	virtual ~MulticrewModule();

	std::string moduleName();
	bool isHostMode();
	bool registered();

protected:
	virtual void receive( void *data, unsigned size )=0;
	void lock();
	void send( void *data, DWORD size, bool safe, Connection::Priority prio, bool append=false );
	void unlock();
	std::string id();

	virtual void sendCompleted( Packet *packet );
	virtual void sendFailed( Packet *packet );
	virtual void sendProc();
	void disconnect();

private:
	friend class MulticrewCore;
	virtual void receive( ModulePacket *packet );
	void connect( SmartPtr<Connection> con );
	DWORD threadProc( LPVOID param );

private:
	struct Data;
	friend Data;
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
	void unprepare();

	void log( std::string line );
	Signal1<const char *> logged;

private:
	MulticrewCore();
	virtual ~MulticrewCore();

	struct Data;
	Data *d;
};

#endif
