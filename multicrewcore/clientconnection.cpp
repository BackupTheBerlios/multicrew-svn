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

#include <PacketEnumerations.h>
#include <RakNetworkFactory.h>
#include <RakClientInterface.h>
#include <NetworkTypes.h>
#include <Multiplayer.h>

#include "streams.h"
#include "log.h"
#include "networkimpl.h"
#include "callback.h"


class ClientConnectionImpl  : public ConnectionImpl,
							  public Multiplayer<RakClientInterface> {
public:
	ClientConnectionImpl( RakClientInterface* client );
	virtual ~ClientConnectionImpl();

	virtual bool sendImpl( char *buf, unsigned len, 
						   PacketPriority priority, 
						   PacketReliability reliability, 
						   char orderingChannel );
	virtual void disconnectImpl();
	virtual void processImpl();

	//Signal connectionAccepted;
	Signal1<Packet*> hostFound;
	RakClientInterface* client;
	volatile bool disconnected;
	volatile bool connected;
	std::string errorMessage;

protected:	
	virtual void ProcessUnhandledPacket(Packet *packet, 
										unsigned char packetIdentifier, 
										RakClientInterface *interfaceType) {
		processPacket( packet->data, packet->length );
	}

	virtual void ReceiveDisconnectionNotification(Packet *packet,RakClientInterface *interfaceType) {
		disconnect();
		derr << "Connection lost normally" << std::endl;
	}

	virtual void ReceiveRemoteDisconnectionNotification(Packet *packet,RakClientInterface *interfaceType) {
		dlog << "Client " 
			 << client->PlayerIDToDottedIP( packet->playerId )
			 << " disconnected"
			 << std::endl;
	}

	virtual void ReceiveRemoteConnectionLost(Packet *packet,RakClientInterface *interfaceType) {
		dlog << "Client " 
			 << client->PlayerIDToDottedIP( packet->playerId )
			 << " kicked"
			 << std::endl;	   
	}
	
	virtual void ReceiveRemoteNewIncomingConnection(Packet *packet,RakClientInterface *interfaceType) {
		dlog << "Client " 
			 << client->PlayerIDToDottedIP( packet->playerId )
			 << " connected"
			 << std::endl;	   
	}
	
	virtual void ReceiveRemoteExistingConnection(Packet *packet,RakClientInterface *interfaceType) {
		dlog << "Other client " 
			 << client->PlayerIDToDottedIP( packet->playerId ) 
			 << " found"
			 << std::endl;
	}

	virtual void ReceiveConnectionBanned(Packet *packet,RakClientInterface *interfaceType) {
		dout << "Received: Banned from this server" << std::endl;
		errorMessage = "Banned from this server";
		client->Disconnect( 300 );
		disconnected = true;
	}

	virtual void ReceiveRemotePortRefused(Packet *packet,RakClientInterface *interfaceType) {
		dout << "Received: Remote port refused" << std::endl;
		errorMessage = "Remote port refused";
		client->Disconnect( 300 );
		disconnected = true;
	}

	virtual void ReceiveNoFreeIncomingConnections(Packet *packet,RakClientInterface *interfaceType) {
		dout << "Received: Sorry, the server is full" << std::endl;
		errorMessage = "Sorry, the server is full";
		client->Disconnect( 300 );
		disconnected = true;
	}

	virtual void ReceiveInvalidPassword(Packet *packet,RakClientInterface *interfaceType) {
		dout << "Received: Invalid password" << std::endl;
		errorMessage = "Invalid password";
		client->Disconnect( 300 );
		disconnected = true;
	}

	virtual void ReceiveModifiedPacket(Packet *packet,RakClientInterface *interfaceType) {
		disconnect();
		dlog << "Packets modified. Cheater!" << std::endl;
	}

	virtual void ReceiveConnectionLost(Packet *packet,RakClientInterface *interfaceType) {
		disconnect();
		derr << "Connection lost" << std::endl;
	}

	virtual void ReceiveConnectionRequestAccepted(Packet *packet,RakClientInterface *interfaceType) {
		dlog << "Connection accepted" << std::endl;
		connected = true;
	}

	virtual void ReceivePong( Packet *packet, RakClientInterface *interfaceType) {
		dout << "Pong" << std::endl;
		hostFound.emit( packet );
	}

};


ClientConnectionImpl::ClientConnectionImpl( RakClientInterface* client ) {
	this->client = client;
	this->disconnected = false;
	this->connected = false;
}


ClientConnectionImpl::~ClientConnectionImpl() {
	if( client!=0 ) {
		client->Disconnect( 300 );
		RakNetworkFactory::DestroyRakClientInterface( client );	
	}
}


bool ClientConnectionImpl::sendImpl( char *buf, unsigned len, 
								   PacketPriority priority, 
								   PacketReliability reliability, 
								   char orderingChannel ) {
	return client->Send( buf, len, priority, reliability, 
						 orderingChannel );
}


void ClientConnectionImpl::disconnectImpl() {
	if( connected ) {
		client->Disconnect( 300 );		
		connected = false;
	}
	disconnected = true;
}


void ClientConnectionImpl::processImpl() {
	ProcessPackets( client );
}


/******************************************************************/


struct ClientConnectionSetup::Data {
	Data( ClientConnectionSetup *setup ) 
		: hostFoundSlot( 0, setup, ClientConnectionSetup::hostFoundCallback ) {
		runningSearch = false;
	}

	~Data() {
	}

	SmartPtr<ClientConnectionImpl> con;
	Slot1<ClientConnectionSetup, Packet*> hostFoundSlot;
	bool runningSearch;
	std::string errorMessage;
};


ClientConnectionSetup::ClientConnectionSetup() {
	d = new Data( this );
}

ClientConnectionSetup::~ClientConnectionSetup() {
	delete d;
}

bool ClientConnectionSetup::init() {
	// create client
	dlog << "Creating client object" << std::endl;
	RakClientInterface *client = RakNetworkFactory::GetRakClientInterface();
	if( client==0 ) {
		dlog << "Failed." << std::endl;
		return false;
	}

	// create connection object
	ClientConnectionImpl *conImpl = new ClientConnectionImpl( client );
	SmartPtr<ClientConnectionImpl> con( conImpl );
	d->hostFoundSlot.connect( &conImpl->hostFound );

	// save for later use
	d->con = conImpl;
	d->con->start();
	return true;
}


bool ClientConnectionSetup::startSearch( bool broadcast, std::string address, int port ) {
	// set hostname and port
	std::string dest;
	if( broadcast )
		dest = "255.255.255.255";
	else
		dest = address;

	// search hosted games
	dlog << "Search hosted games" << std::endl;
	d->runningSearch = true;
	d->con->client->PingServer( (char*)dest.c_str(), port, 0, true);
	newSearch.emit();

	return true;
}


void ClientConnectionSetup::stopSearch() {
	d->runningSearch = false;
}


class ClientConnectionSetup::FoundHostImpl : public ClientConnectionSetup::FoundHost {
public:
	FoundHostImpl( std::string desc, int latency, PlayerID *player ) {
		this->desc = desc;
		this->latencyMS = latency;
		this->player = *player;
	}

	virtual ~FoundHostImpl() {
	}

	std::string description() { return desc; }
	int latency() { return latencyMS; }

	int latencyMS;
	std::string desc;
	PlayerID player;
};


void ClientConnectionSetup::hostFoundCallback( Packet *packet ) {
	if( packet!=0 && d->runningSearch ) {
		// create description
		std::string desc( d->con->client->PlayerIDToDottedIP( packet->playerId ) );
		
		// send signal
		hostFound.emit( SmartPtr<FoundHost>(
			new FoundHostImpl( 
				desc, 
				*(int*)(packet->data+1),
				&packet->playerId )));
	}
}


SmartPtr<Connection> ClientConnectionSetup::connect( SmartPtr<FoundHost> host ) {
	FoundHostImpl *hostImpl = (FoundHostImpl*)(&*host);

	dlog << "Connect to " << hostImpl->desc 
		 << ":" << hostImpl->player.port
		 << std::endl;

	// setup connection state
	d->errorMessage = "";
	d->con->errorMessage = "";
	d->con->connected = false;
	d->con->disconnected = false;

	// connect to host
	std::string dest = d->con->client->PlayerIDToDottedIP( hostImpl->player );
	bool ok = d->con->client->Connect(
		(char*)dest.c_str(),
		hostImpl->player.port,
		0,
		0,
		1 );
	if( !ok ) {
		dlog << "Failed." << std::endl;
		return 0;
	}

	// wait for answer
	dout << "Waiting for answer" << std::endl;
	int timeout = 3000;
	while( !d->con->connected && !d->con->disconnected && timeout>0 ) {
		dlog << "Connecting (" << timeout << "ms)" << std::endl;
		Sleep( 100 );
		timeout-=100;
	}

	if( d->con->connected ) {
		dout << "Connected" << std::endl;
		return d->con;
	}

	if( !d->con->disconnected )
		d->errorMessage = "Connection timeout";
	else
		if( d->con->errorMessage.length()>0 )
			d->errorMessage = d->con->errorMessage;
		else
			d->errorMessage = "Unknown connection error";
	return 0;
}


std::string ClientConnectionSetup::errorMessage() {
	if( d->errorMessage.length()>0 )
		return d->errorMessage;
	else
		return "No error";
}
