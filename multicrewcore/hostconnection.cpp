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

#include <windows.h>

#include "../RakNet/source/PacketEnumerations.h"
#include "../RakNet/source/RakNetworkFactory.h"
#include "../RakNet/source/RakClientInterface.h"
#include "../RakNet/source/RakServerInterface.h"
#include "../RakNet/source/NetworkTypes.h"
#include "../RakNet/source/Multiplayer.h"

#include "streams.h"
#include "log.h"
#include "config.h"
#include "network.h"
#include "callback.h"


class HostConnectionImpl : public Connection,
						   public Multiplayer<RakServerInterface>
{
public:
	HostConnectionImpl( RakServerInterface* server );
	virtual ~HostConnectionImpl();

protected:
	virtual bool sendImpl( char *buf, unsigned len, 
					  PacketPriority priority, 
					  PacketReliability reliability, 
					  unsigned orderingChannel );
	virtual void disconnectImpl(); 
	virtual void processImpl();
	virtual void ProcessUnhandledPacket(Packet *packet, 
										unsigned char packetIdentifier, 
										RakServerInterface *interfaceType);

	virtual void ReceiveRemoteDisconnectionNotification(Packet *packet,RakServerInterface *interfaceType) {
		dlog << "Client " 
			 << client->PlayerIDToDottedIP( packet->playerId )
			 << " disconnected"
			 << std::endl;	   
	}

	virtual void ReceiveDisconnectionNotification(Packet *packet,RakServerInterface *interfaceType) {
		dlog << "Disconnected" << std::endl;
		//disconnect();
	}

	virtual void ReceiveNewIncomingConnection(Packet *packet,RakServerInterface *interfaceType) {
		dlog << "Incoming connection from " 
			 << client->PlayerIDToDottedIP( packet->playerId ) 
			 << std::endl;
	}
	
	virtual void ReceiveModifiedPacket(Packet *packet,RakServerInterface *interfaceType) {
		dlog << "Modified packet. Cheater!" << std::endl;
		disconnect();
	}
		
	virtual void ReceiveConnectionLost(Packet *packet,RakServerInterface *interfaceType) {
		dlog << "Connection lost" << std::endl;
		//disconnect();
	}

private:
	RakServerInterface *server;
	RakClientInterface *client;
};


HostConnectionImpl::HostConnectionImpl( RakServerInterface* server ) {
	this->server = server;
	this->client = RakNetworkFactory::GetRakClientInterface(); // just for the dotted string function
}


HostConnectionImpl::~HostConnectionImpl() {
	if( server!=0 ) {
		server->Disconnect( 300 );
		RakNetworkFactory::DestroyRakServerInterface( server );	
	}
	stopThread();
	RakNetworkFactory::DestroyRakClientInterface( client );	
}


bool HostConnectionImpl::sendImpl( char *buf, unsigned len, 
								   PacketPriority priority, 
								   PacketReliability reliability, 
								   unsigned orderingChannel ) {
	if( server->GetConnectedPlayers()>0 ) {
		return server->Send( buf, len, priority, reliability, 
							 orderingChannel,
							 UNASSIGNED_PLAYER_ID, true );
	} else
		return true;
}


void HostConnectionImpl::disconnectImpl() {
	server->Disconnect( 300 );
}


void HostConnectionImpl::processImpl() {
	ProcessPackets( server );
}


void HostConnectionImpl::ProcessUnhandledPacket(Packet *packet, 
												unsigned char packetIdentifier, 
												RakServerInterface *interfaceType) {
	processPacket( packet->data, packet->length );
}


/***********************************************************************/

struct HostConnectionSetup::Data {
	RakServerInterface* server;
};

HostConnectionSetup::HostConnectionSetup() {
	d = new Data;
	d->server = 0;
}

HostConnectionSetup::~HostConnectionSetup() {
	dout << "~HostConnectionSetup()" << std::endl;
	if( d->server!=0 )
		RakNetworkFactory::DestroyRakServerInterface( d->server );
	delete d;
}

SmartPtr<Connection> HostConnectionSetup::host( int port, 
												std::string sessionName, 
												bool passwordEnabled, 
												std::string password  ) {
	// create server object
	dlog << "Creating server object" << std::endl;
	d->server = RakNetworkFactory::GetRakServerInterface();
	if( d->server==0 ) {
		dlog << "Failed." << std::endl;
		return 0;
	}

	// setup connection parameters
	if( passwordEnabled ) d->server->SetPassword( (char*)password.c_str() );
	//d->server->DisableSecurity();

	// host the game
	dlog << "Hosting session" << std::endl;
	bool ok = d->server->Start( 8, 0, 50, port );
	if( !ok ) {
		dlog << "Failed" << std::endl;
		return 0;
	}

	// create connection object
	SmartPtr<Connection> con( new HostConnectionImpl( d->server ) );
	con->start();
	d->server = 0;

	return con;
}
