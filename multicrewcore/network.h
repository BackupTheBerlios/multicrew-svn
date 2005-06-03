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

#ifndef MULTICREWCORE_NETWORK_H_INCLUDED
#define MULTICREWCORE_NETWORK_H_INCLUDED

#include "common.h"

#pragma warning (disable : 4075 4275)

#include <map>
#include <string>

#include <PacketPriority.h>

#include "shared.h"
#include "signals.h"
#include "packets.h"
#include "network.h"
#include "thread.h"

#define MULTICREW_PORT "1299"


enum Priority { highPriority, mediumPriority, lowPriority };


class Connection : public Shared, public Thread {
public:
	Connection();
	virtual ~Connection();

	virtual bool start();
	virtual bool send( SmartPtr<PacketBase> packet, bool safe, 
					   Priority prio, unsigned channel );
	virtual void disconnect();
	Signal disconnected;

protected:
	virtual bool sendImpl( char *buf, unsigned len, 
						   PacketPriority priority, 
						   PacketReliability reliability, 
						   unsigned orderingChannel )=0;
	virtual void disconnectImpl()=0;
	virtual void processImpl()=0;
	virtual unsigned threadProc( void *param );
	void processPacket( void *data, unsigned length );

	struct Data;
	friend Data;
	Data *d;
};


class DLLEXPORT HostConnectionSetup {
public:
	HostConnectionSetup();
	virtual ~HostConnectionSetup();

	SmartPtr<Connection> host( int port, std::string sessionName, 
							   bool passwordEnabled, std::string password );

private:
	struct Data;
	Data *d;
};


class DLLEXPORT ClientConnectionSetup {
public:
	ClientConnectionSetup();
	virtual ~ClientConnectionSetup();

	bool init();
	bool startSearch( bool broadcast, std::string address, int port );
	void stopSearch();

	class FoundHost : public Shared {
	public:
		virtual int latency()=0;
		virtual std::string description()=0;
	};

	Signal newSearch;
	typedef SmartPtr<FoundHost> SmartFoundHost;
	Signal1<SmartFoundHost> hostFound;

	SmartPtr<Connection> connect( SmartPtr<FoundHost> host );
	std::string errorMessage();

private:
	void hostFoundCallback( struct Packet * );

	struct Data;
	Data *d;
	friend Data;
	class FoundHostImpl;
};




#endif
