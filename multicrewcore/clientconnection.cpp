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
#include <dplay8.h>

#include "log.h"
#include "debug.h"
#include "error.h"
#include "network.h"
#include "callback.h"


class ClientConnectionImpl  : public ClientConnection {
public:
	ClientConnectionImpl( IDirectPlay8Peer *peer );
	virtual ~ClientConnectionImpl();

	virtual void disconnect();
	virtual void start();
	virtual bool send( Packet *packet, bool safe, bool sync=false );

	Signal1<DPNMSG_ENUM_HOSTS_RESPONSE*> hostFound;
	PFNDPNMESSAGEHANDLER callback();

	HRESULT callback( PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage );

private:
	struct Data;
	Data *d;
};

struct ClientConnectionImpl::Data {
	Data( ClientConnectionImpl *con ) 
		: callbackAdapter( con, ClientConnectionImpl::callback ) {
	}

	DPNID clientGroup;
	DPNID hostGroup;
	DPNID clientPlayer;
	IDirectPlay8Peer *peer;
	CallbackAdapter3<HRESULT, class ClientConnectionImpl, 
					 PVOID, DWORD, PVOID> callbackAdapter;
};

ClientConnectionImpl::ClientConnectionImpl( IDirectPlay8Peer *peer ) {
	d = new Data( this );
	d->peer = peer;
	d->hostGroup = 0;
	d->clientGroup = 0;
	d->clientPlayer = 0;
	d->peer->AddRef();
}

ClientConnectionImpl::~ClientConnectionImpl() {
	if( d->peer!=0 ) d->peer->Release();
	delete d;
}

PFNDPNMESSAGEHANDLER ClientConnectionImpl::callback() {
	return d->callbackAdapter.callback();
}

HRESULT ClientConnectionImpl::callback( PVOID pvUserContext, DWORD dwMessageType, 
										PVOID pMessage ) {
	dout << "DirectPlay message of type " << dwMessageType << std::endl;
	
	switch( dwMessageType ) {
	case DPN_MSGID_RECEIVE: 
	{
		DPNMSG_RECEIVE *rec = (DPNMSG_RECEIVE*)pMessage;
		Packet *packet = (Packet*)rec->pReceiveData;
		dout << "Received packet of type " << packet->id << std::endl;
		if( packet->id>=firstModulePacket )
			deliverModulePacket( (ModulePacket*)packet );
	}
	break;
			
	case DPN_MSGID_CREATE_GROUP: 
	{
		// group creation message, either host or client group
		DPNMSG_CREATE_GROUP *info = (DPNMSG_CREATE_GROUP*)pMessage;
		dout << "Getting group info for " << info->dpnidGroup << std::endl;
		DPN_GROUP_INFO *groupInfo = new DPN_GROUP_INFO;
		groupInfo->dwSize = sizeof(DPN_GROUP_INFO);
		DWORD size = sizeof(DPN_GROUP_INFO);
		HRESULT hr = d->peer->GetGroupInfo( 
			info->dpnidGroup,
			groupInfo,
			&size,
			0);
		if( hr==DPNERR_BUFFERTOOSMALL ) {
			delete groupInfo;
			groupInfo = (DPN_GROUP_INFO *)new char[size];
			groupInfo->dwSize = sizeof(DPN_GROUP_INFO);
			hr = d->peer->GetGroupInfo( 
				info->dpnidGroup,
				groupInfo,
				&size,
				0);
		}
		if( FAILED(hr) ) {
			log << "Failed: " << fe(hr).c_str() << std::endl;
			delete groupInfo;
			return S_OK;
		}
		
		// copy info
		char name[1024];
		wcstombs( name, groupInfo->pwszName, 1024 );
		dout << "Group \"" << name << "\" created." << std::endl;
		if( strcmp(name, "Host" ) )
			d->hostGroup = info->dpnidGroup;
		else if( strcmp(name, "Client" ) ) {
			d->clientGroup = info->dpnidGroup;

			if( d->clientPlayer!=0 ) {
				// add to client group
				dout << "Add to client group" << std::endl;
				HRESULT hr = d->peer->AddPlayerToGroup(
					d->clientGroup,
					d->clientPlayer,
					NULL,
					NULL,
					DPNADDPLAYERTOGROUP_SYNC );
				if( FAILED(hr) ) {
					log << "Failed: " << fe(hr).c_str() << std::endl;
					return false;
				}
			}
		}
		
		// delete buffer
		delete groupInfo;
	}
	break;
		
		
	case DPN_MSGID_CREATE_PLAYER: 
	{
		// player found, the first is the client
		DPNMSG_CREATE_PLAYER *player = (DPNMSG_CREATE_PLAYER*)pMessage;
		dout << "New player " << player->dpnidPlayer << std::endl;
		if( d->clientPlayer==0 ) {
			dout << "Oh, that's me." << std::endl;
			d->clientPlayer = player->dpnidPlayer;
		} else 
			
		if( d->clientGroup!=0 ) {
			// add to client group
			dout << "Add to client group" << std::endl;
			HRESULT hr = d->peer->AddPlayerToGroup(
				d->clientGroup,
				player->dpnidPlayer,
				NULL,
				NULL,
				DPNADDPLAYERTOGROUP_SYNC );
			if( FAILED(hr) ) {
				dout << "Failed: " << fe(hr).c_str() << std::endl;
				return false;
			}
		}
	}
	break;
	
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		hostFound.emit( (DPNMSG_ENUM_HOSTS_RESPONSE*)pMessage );
		break;
		
	};
	return S_OK;
}

void ClientConnectionImpl::disconnect() {
	log << "Closing peer" << std::endl;
	d->peer->Close( 0 );
	
	SmartPtr<ClientConnectionImpl> myself( this ); // make sure this is not deleted in the signal
	disconnected.emit();
}

void ClientConnectionImpl::start() {
}

bool ClientConnectionImpl::send( Packet *packet, bool safe, bool sync ) {
	dout << "Sending packet of type " << packet->id << std::endl;
	
	DPNHANDLE asyncHandle;
	DPN_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(DPN_BUFFER_DESC) );
	desc.pBufferData = (BYTE*)packet;
	desc.dwBufferSize = packet->size;
	HRESULT hr = d->peer->SendTo(
		d->hostGroup, //DPNID_ALL_PLAYERS_GROUP,
		&desc,
		1,
		0,
		NULL,
		&asyncHandle,
		(sync?DPNSEND_SYNC:0) |
		(safe?DPNSEND_GUARANTEED:0) |
		DPNSEND_NOLOOPBACK | DPNSEND_COALESCE );
	if( FAILED(hr) ) {
		dout << "Failed to send packet: " << fe(hr).c_str() << std::endl;
		return false;
	}
	
	return true;
}

/******************************************************************/

struct ClientConnectionSetup::Data {
	Data( ClientConnectionSetup *setup ) 
		: hostFoundSlot( 0, setup, ClientConnectionSetup::hostFoundCallback ) {
		peer = 0;
		deviceAddress = 0;
		destAddress = 0;
		runningSearch = 0;
	}

	~Data() {
		if( deviceAddress )
			deviceAddress->Release();

		if( destAddress )
			destAddress->Release();

		if( peer ) {
			peer->Release();
		}
	}

	IDirectPlay8Peer *peer;
	IDirectPlay8Address* deviceAddress;
	IDirectPlay8Address* destAddress;
	DPNHANDLE runningSearch;
	SmartPtr<ClientConnectionImpl> con;
	Slot1<ClientConnectionSetup, DPNMSG_ENUM_HOSTS_RESPONSE*> hostFoundSlot;
};

ClientConnectionSetup::ClientConnectionSetup() {
	d = new Data( this );
}

ClientConnectionSetup::~ClientConnectionSetup() {
	if( d->runningSearch ) stopSearch();

	
	delete d;
}

bool ClientConnectionSetup::init() {
	HRESULT hr;

	// create directplay peer object
	log << "Creating DirectPlay8Peer object" << std::endl;
	hr = CoCreateInstance( 
		CLSID_DirectPlay8Peer, NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlay8Peer, 
        (LPVOID*) &d->peer );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// create connection object
	ClientConnectionImpl *conImpl = new ClientConnectionImpl( d->peer );
	SmartPtr<ClientConnection> con( conImpl );
	d->hostFoundSlot.connect( &conImpl->hostFound );

	// initialize
	log << "Initializing peer" << std::endl;
	hr = d->peer->Initialize(NULL, conImpl->callback(), 0 );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// Create our IDirectPlay8Address Device Address
	log << "Creating device address" << std::endl;
	hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDirectPlay8Address,
                                       (LPVOID*) &d->deviceAddress );
    if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str()  << std::endl;
        return false;
    }
    
    // Set the SP for our Device Address
	log << "Setting service provider to TCPIP" << std::endl;
	hr = d->deviceAddress->SetSP(&CLSID_DP8SP_TCPIP );
    if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
        return false;
    }

	// save for later use
	d->con = conImpl;

	return true;
}


bool ClientConnectionSetup::startSearch( bool broadcast, std::string address, int port ) {
	if( d->runningSearch ) stopSearch();
	if( d->destAddress ) {
		d->destAddress->Release();
		d->destAddress = 0;
	}

	// set hostname and port
	if( !broadcast ) {
		// set destination address
		log << "Creating destination address" << std::endl;
		HRESULT hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlay8Address,
										(LPVOID*) &d->destAddress );
		if( FAILED(hr) ) {
			log << "Failed with: " << fe(hr).c_str()  << std::endl;
			return false;
		}

		// Set the SP for the destination Address
		log << "Setting service provider to TCPIP for destination" << std::endl;
		hr = d->destAddress->SetSP(&CLSID_DP8SP_TCPIP );
		if( FAILED(hr) ) {
			log << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}

		log << "Set address to " << address.c_str() << std::endl;
		d->destAddress->AddComponent( DPNA_KEY_HOSTNAME,
			address.c_str(), address.length()+1, DPNA_DATATYPE_STRING );
		if( FAILED(hr) ) {
			log << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}

		log << "Set port to " << port << std::endl;
		d->destAddress->AddComponent( DPNA_KEY_PORT, 
			&port, sizeof(DWORD), DPNA_DATATYPE_DWORD );
		if( FAILED(hr) ) {
			log << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}
	}

	// search hosted games
	DPN_APPLICATION_DESC appDesc;
    ZeroMemory( &appDesc, sizeof(DPN_APPLICATION_DESC) );
    appDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    appDesc.guidApplication = gMulticrewGuid;
	
	log << "Search hosted games" << std::endl;
	HRESULT hr = d->peer->EnumHosts(
		&appDesc,
		d->destAddress,
		d->deviceAddress,
		0,
		0,
		0,
		INFINITE,
		0,
		0,
		&d->runningSearch,
		0 ); //DPNENUMHOSTS_OKTOQUERYFORADDRESSING );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}
	dout << "EnumHost returned" << std::endl;

	newSearch.emit();

	return true;
}


void ClientConnectionSetup::stopSearch() {
	if( d->runningSearch ) {
		d->peer->CancelAsyncOperation( d->runningSearch, DPNCANCEL_ENUM );
		d->runningSearch = 0;
	}
}


class ClientConnectionSetup::FoundHostImpl : public ClientConnectionSetup::FoundHost {
public:
	FoundHostImpl( std::wstring desc, int latency, 
		IDirectPlay8Address *hostAddress, IDirectPlay8Address *deviceAddress ) {
		this->desc = desc;
		this->latencyMS = latency;
		this->hostAddress = hostAddress;
		this->deviceAddress = deviceAddress;
		hostAddress->AddRef();
		deviceAddress->AddRef();
	}

	virtual ~FoundHostImpl() {
		hostAddress->Release();
		deviceAddress->Release();
	}

	std::wstring description() { return desc; }
	int latency() { return latencyMS; }

	int latencyMS;
	std::wstring desc;
	IDirectPlay8Address *hostAddress;
	IDirectPlay8Address *deviceAddress;
};


void ClientConnectionSetup::hostFoundCallback( _DPNMSG_ENUM_HOSTS_RESPONSE *resp ) {
	if( resp!=0 ) {
		// create description
		std::wstring desc( resp->pApplicationDescription->pwszSessionName );
		char msg[1024];
		wcstombs( msg, desc.c_str(), 1024 );
		log << "Host found: " << msg << std::endl;

		// send signal
		hostFound.emit( SmartPtr<FoundHost>(
			new FoundHostImpl( 
				desc, 
				resp->dwRoundTripLatencyMS,
				resp->pAddressSender,
				resp->pAddressDevice )));
	}
}


SmartPtr<ClientConnection> ClientConnectionSetup::connect( SmartPtr<FoundHost> host ) {
	char desc[1024];
	wcstombs( desc, host->description().c_str(), 1024 );

	log << "Connect to \"" << desc << "\"" << std::endl;
	// connect to host
	FoundHostImpl *hostImpl = (FoundHostImpl*)(&*host);
	DPN_APPLICATION_DESC appDesc;
    ZeroMemory( &appDesc, sizeof(DPN_APPLICATION_DESC) );
    appDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    appDesc.guidApplication = gMulticrewGuid;
	HRESULT hr = d->peer->Connect(
		&appDesc,
		hostImpl->hostAddress,
		hostImpl->deviceAddress,
		NULL,
		NULL,
		NULL,
		0,
		NULL,
		NULL,
		NULL,
		DPNCONNECT_OKTOQUERYFORADDRESSING | DPNCONNECT_SYNC);
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return 0;
	}

	return d->con;
}
