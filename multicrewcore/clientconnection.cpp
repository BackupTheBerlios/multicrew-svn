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

#include "streams.h"
#include "log.h"
#include "networkimpl.h"
#include "callback.h"


class ClientConnectionImpl  : public ConnectionImpl {
public:
	ClientConnectionImpl( IDirectPlay8Peer *peer );
	virtual ~ClientConnectionImpl();
	
	Signal1<DPNMSG_ENUM_HOSTS_RESPONSE*> hostFound;

protected:	
	virtual HRESULT messageCallback( PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage );
};


ClientConnectionImpl::ClientConnectionImpl( IDirectPlay8Peer *peer )
	: ConnectionImpl( peer ) {
}


ClientConnectionImpl::~ClientConnectionImpl() {
}


HRESULT ClientConnectionImpl::messageCallback( PVOID pvUserContext, DWORD dwMessageType, 
										PVOID pMessage ) {
	switch( dwMessageType ) {
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		hostFound.emit( (DPNMSG_ENUM_HOSTS_RESPONSE*)pMessage );
		break;

	default:
		return ConnectionImpl::messageCallback( pvUserContext, dwMessageType, pMessage );
	};

	return S_OK;
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
	dlog << "Creating DirectPlay8Peer object" << std::endl;
	hr = CoCreateInstance( 
		CLSID_DirectPlay8Peer, NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlay8Peer, 
        (LPVOID*) &d->peer );
	if( FAILED(hr) ) {
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// create connection object
	ClientConnectionImpl *conImpl = new ClientConnectionImpl( d->peer );
	SmartPtr<ClientConnectionImpl> con( conImpl );
	d->hostFoundSlot.connect( &conImpl->hostFound );

	// initialize
	dlog << "Initializing peer" << std::endl;
	hr = d->peer->Initialize(NULL, conImpl->callback(), 0 );
	if( FAILED(hr) ) {
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// Create our IDirectPlay8Address Device Address
	dlog << "Creating device address" << std::endl;
	hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDirectPlay8Address,
                                       (LPVOID*) &d->deviceAddress );
    if( FAILED(hr) ) {
		dlog << "Failed: " << fe(hr).c_str()  << std::endl;
        return false;
    }
    
    // Set the SP for our Device Address
	dlog << "Setting service provider to TCPIP" << std::endl;
	hr = d->deviceAddress->SetSP(&CLSID_DP8SP_TCPIP );
    if( FAILED(hr) ) {
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
        return false;
    }

	// set player info
	DPN_PLAYER_INFO playerInfo;
    ZeroMemory( &playerInfo, sizeof(DPN_PLAYER_INFO) );
	playerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
	playerInfo.dwInfoFlags = DPNINFO_NAME;
	wchar_t name[16];
	mbstowcs( name, "Client", 16 );
	playerInfo.pwszName = name;
	dlog << "Setting player information" << std::endl;
	hr = d->peer->SetPeerInfo( 
		&playerInfo,
		NULL,
		NULL,
		DPNSETPEERINFO_SYNC );
	if( FAILED(hr) ) {
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
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
		dlog << "Creating destination address" << std::endl;
		HRESULT hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
										CLSCTX_INPROC_SERVER,
										IID_IDirectPlay8Address,
										(LPVOID*) &d->destAddress );
		if( FAILED(hr) ) {
			dlog << "Failed with: " << fe(hr).c_str()  << std::endl;
			return false;
		}

		// Set the SP for the destination Address
		dlog << "Setting service provider to TCPIP for destination" << std::endl;
		hr = d->destAddress->SetSP(&CLSID_DP8SP_TCPIP );
		if( FAILED(hr) ) {
			dlog << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}

		dlog << "Set address to " << address.c_str() << std::endl;
		d->destAddress->AddComponent( DPNA_KEY_HOSTNAME,
			address.c_str(), address.length()+1, DPNA_DATATYPE_STRING );
		if( FAILED(hr) ) {
			dlog << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}

		dlog << "Set port to " << port << std::endl;
		d->destAddress->AddComponent( DPNA_KEY_PORT, 
			&port, sizeof(DWORD), DPNA_DATATYPE_DWORD );
		if( FAILED(hr) ) {
			dlog << "Failed: " << fe(hr).c_str() << std::endl;
			return false;
		}
	}

	// search hosted games
	DPN_APPLICATION_DESC appDesc;
    ZeroMemory( &appDesc, sizeof(DPN_APPLICATION_DESC) );
    appDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    appDesc.guidApplication = gMulticrewGuid;
	
	dlog << "Search hosted games" << std::endl;
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
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
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
		dlog << "Host found: " << msg << std::endl;

		// send signal
		hostFound.emit( SmartPtr<FoundHost>(
			new FoundHostImpl( 
				desc, 
				resp->dwRoundTripLatencyMS,
				resp->pAddressSender,
				resp->pAddressDevice )));
	}
}


SmartPtr<Connection> ClientConnectionSetup::connect( SmartPtr<FoundHost> host ) {
	char desc[1024];
	wcstombs( desc, host->description().c_str(), 1024 );

	dlog << "Connect to \"" << desc << "\"" << std::endl;
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
		dlog << "Failed: " << fe(hr).c_str() << std::endl;
		return 0;
	}

	return d->con;
}
