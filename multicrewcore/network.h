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

#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include "common.h"

#include <map>
#include <string>

#include "shared.h"
#include "signals.h"
#include "packets.h"

extern const GUID gMulticrewGuid;
#define MULTICREW_PORT "1299"

class Receiver {
public:
	virtual std::string id()=0;
	virtual void receive( ModulePacket *packet )=0;
};


class Connection : public Shared {
public:
	virtual bool send( Packet *packet, bool safe, bool sync=false )=0;

	void addReceiver( Receiver *receiver );
	void removeReceiver( Receiver *receiver );

protected:
	std::map<std::string,Receiver*> receivers;
	void deliverModulePacket( ModulePacket *packet );
};


class HostConnection : public Connection {
public:
	virtual void disconnect()=0;
	Signal disconnected;

	virtual bool send( Packet *packet, bool safe, bool sync=false )=0;

	virtual void start()=0;
};


class DLLEXPORT HostConnectionSetup {
public:
	HostConnectionSetup();
	virtual ~HostConnectionSetup();

	SmartPtr<HostConnection> host( int port, std::wstring sessionName, bool passwordEnabled, std::wstring password );

private:
	struct Data;
	Data *d;
};


class ClientConnection : public Connection {
public:
	virtual void disconnect()=0;
	Signal disconnected;

	virtual bool send( Packet *packet, bool safe, bool sync=false )=0;

	virtual void start()=0;
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
		virtual std::wstring description()=0;
	};

	typedef SmartPtr<FoundHost> SmartFoundHost;

	//DLLEXPORT_TEMPLATE template class DLLEXPORT Signal1<SmartFoundHost>;

	Signal newSearch;
	Signal1<SmartFoundHost> hostFound;

	SmartPtr<ClientConnection> connect( SmartPtr<FoundHost> host );

private:
	void hostFoundCallback( struct _DPNMSG_ENUM_HOSTS_RESPONSE * );

	struct Data;
	Data *d;
	friend Data;
	class FoundHostImpl;
};




#endif