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

#include "../RakNet/source/PacketPriority.h"

#include "shared.h"
#include "signals.h"
#include "packets.h"
#include "network.h"
#include "thread.h"

#define MULTICREW_PORT "1299"


enum Priority { highPriority, mediumPriority, lowPriority };


class Connection : public Shared, public Thread, private AsyncCallee {
public:
	Connection();
	virtual ~Connection();

	enum State {
		idleState,
		connectingState,
		connectedState,
		disconnectedState
	};
	State state();
	
	virtual bool send( SmartPtr<PacketBase> packet, bool safe, 
					   Priority prio, unsigned channel );
	
	virtual void disconnect()=0;
	Signal1<std::string /*reason*/> disconnected;
	std::string error();

 protected:
	virtual bool sendImpl( char *buf, unsigned len, 
						   PacketPriority priority, 
						   PacketReliability reliability, 
						   unsigned orderingChannel )=0;
	void setState( State state, std::string error="" );
	void processPacket( void *data, unsigned length );

	virtual void processImpl()=0;

 private:
	unsigned threadProc( void *param );
	void asyncCallback();

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


class DLLEXPORT NetworkChannel : public PacketFactory<PacketBase> {
 public:
	NetworkChannel( std::string id );
	virtual ~NetworkChannel();
	std::string channelId();
	unsigned channelNum();
	void setChannelNum( unsigned num );

	bool send( SmartPtr<PacketBase> packet, bool safe, Priority prio );
	virtual void receive( SmartPtr<PacketBase> packet )=0;
	virtual void sendFullState()=0;

 private:
	struct Data;
	friend Data;
	Data *d;
};


class DLLEXPORT ChannelProtocol : private StringTypedPacketFactory<PacketBase> {
 public:
	ChannelProtocol();
	virtual ~ChannelProtocol();

	enum Mode {
		IdleMode,
		HostMode,
		ClientMode,
	};

	virtual Mode mode();
	Signal1<Mode> modeChanged;
	virtual void start( bool host, SmartPtr<Connection> con );
	virtual void stop();
	void sendFullState();

 protected:
	friend NetworkChannel;
	unsigned registerNetworkChannel( NetworkChannel *channel );
	void unregisterNetworkChannel( NetworkChannel *channel );

 protected:
	friend Connection;
	void receive( SharedBuffer &buf );

 private:
	SmartPtr<PacketBase> createPacket( std::string channel, SharedBuffer &buffer );
	void receiveChannelPacket( std::string channel, SharedBuffer &buffer );
	void receiveChannelPacket( std::string channel, SmartPtr<PacketBase> packet );
	bool sendChannelPacket( SmartPtr<PacketBase> packet, bool safe,
							Priority prio, NetworkChannel *channel );

	unsigned getIdNum( std::string id );
	std::string getNumId( unsigned num );
	unsigned createIdNum( std::string id );
	void setIdNum( std::string id, unsigned num );

	void handleFullState();

	struct Data;
	Data *d;
};


#endif
