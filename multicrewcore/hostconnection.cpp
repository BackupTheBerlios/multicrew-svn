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
#include "config.h"
#include "error.h"
#include "networkimpl.h"
#include "callback.h"


const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP = 
{ 0x8d3f9e5e, 0xa3bd, 0x475b, 0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c };


class HostConnectionImpl : public ConnectionImpl {
public:
	HostConnectionImpl( IDirectPlay8Peer *peer );
	virtual ~HostConnectionImpl();

protected:
	virtual HRESULT messageCallback( PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage );
};


HostConnectionImpl::HostConnectionImpl( IDirectPlay8Peer *peer )
	: ConnectionImpl( peer ) {
}


HostConnectionImpl::~HostConnectionImpl() {
}


HRESULT HostConnectionImpl::messageCallback( PVOID pvUserContext, DWORD dwMessageType, 
											 PVOID pMessage ){
//	dout << "HostConnectionImpl messageCallback" << std::endl;
	return ConnectionImpl::messageCallback( pvUserContext, dwMessageType, pMessage );
}


/***********************************************************************/

struct HostConnectionSetup::Data {
	IDirectPlay8Peer *peer;
	IDirectPlay8Address* deviceAddress;
};

HostConnectionSetup::HostConnectionSetup() {
	d = new Data;
	d->peer = 0;
	d->deviceAddress = 0;
}

HostConnectionSetup::~HostConnectionSetup() {
	dout << "~HostConnectionSetup()" << std::endl;
	if( d->deviceAddress )
		d->deviceAddress->Release();

	if( d->peer ) {
		d->peer->Release();
	}
	delete d;
}

SmartPtr<Connection> HostConnectionSetup::host( int port, 
												std::wstring sessionName, 
												bool passwordEnabled, 
												std::wstring password  ) {
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
		return 0;
	}

	// create connection object
	HostConnectionImpl *conImpl = new HostConnectionImpl( d->peer );
	SmartPtr<Connection> con( conImpl );

	// initialize
	log << "Initializing peer" << std::endl;
	hr = d->peer->Initialize(NULL, conImpl->callback(), 0 );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return 0;
	}

	// Create our IDirectPlay8Address Device Address
	log << "Creating device address" << std::endl;
	hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IDirectPlay8Address,
                                       (LPVOID*) &d->deviceAddress );
    if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str()  << std::endl;
        return 0;
    }

	log << "Set port to " << port << std::endl;
	d->deviceAddress->AddComponent( DPNA_KEY_PORT, 
		&port, sizeof(DWORD), DPNA_DATATYPE_DWORD );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return 0;
    }
    
    // Set the SP for our Device Address
	log << "Setting service provider to TCPIP" << std::endl;
	bool useSim = Config::config()->boolValue( "", "DirectPlaySimulator", false );
	if( useSim ) {
		hr = d->deviceAddress->SetSP( &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP );
		if( FAILED(hr) )
			hr = d->deviceAddress->SetSP( &CLSID_DP8SP_TCPIP );
		else
			dout << "Using network simulator" << std::endl;
	} else
		hr = d->deviceAddress->SetSP( &CLSID_DP8SP_TCPIP );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return 0;
	}

	// set player info
	DPN_PLAYER_INFO playerInfo;
    ZeroMemory( &playerInfo, sizeof(DPN_PLAYER_INFO) );
	playerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
	playerInfo.dwInfoFlags = DPNINFO_NAME;
	wchar_t name[16];
	mbstowcs( name, "Host", 16 );
	playerInfo.pwszName = name;
	log << "Setting player information" << std::endl;
	hr = d->peer->SetPeerInfo( 
		&playerInfo,
		NULL,
		NULL,
		DPNSETPEERINFO_SYNC );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// host the game
	DPN_APPLICATION_DESC appDesc;
    ZeroMemory( &appDesc, sizeof(DPN_APPLICATION_DESC) );
    appDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    appDesc.guidApplication = gMulticrewGuid;
	appDesc.pwszSessionName = (WCHAR*)sessionName.c_str();
	if( passwordEnabled ) {
		appDesc.pwszPassword = (WCHAR*)password.c_str();
		appDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD; 
	}

	log << "Hosting session" << std::endl;
	hr = d->peer->Host( 
		&appDesc,            // AppDesc
		&d->deviceAddress, 1,  // Device Address
        NULL, NULL,          // Reserved
        0,                // Player Context
        0 );                 // dwFlags
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// create host group
	log << "Creating host group" << std::endl;
	//wchar_t name[16];
	mbstowcs( name, "Host", 16 );
	DPN_GROUP_INFO groupInfo;
	ZeroMemory( &groupInfo, sizeof(DPN_GROUP_INFO) );
	groupInfo.dwSize = sizeof(DPN_GROUP_INFO);
	groupInfo.dwInfoFlags = DPNINFO_NAME;
	groupInfo.pwszName = name;
	groupInfo.dwGroupFlags = DPNGROUP_AUTODESTRUCT;
	hr = d->peer->CreateGroup(
		&groupInfo,
		NULL,
		NULL,
		0,
		DPNCREATEGROUP_SYNC );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// create client group
	log << "Creating client group" << std::endl;
	mbstowcs( name, "Client", 16 );
	ZeroMemory( &groupInfo, sizeof(DPN_GROUP_INFO) );
	groupInfo.dwSize = sizeof(DPN_GROUP_INFO);
	groupInfo.dwInfoFlags = DPNINFO_NAME;
	groupInfo.pwszName = name;
	groupInfo.dwGroupFlags = DPNGROUP_AUTODESTRUCT;
	hr = d->peer->CreateGroup(
		&groupInfo,
		NULL,
		NULL,
		0,
		DPNCREATEGROUP_SYNC );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// join host group
	log << "Joining host group" << std::endl;
	hr = d->peer->AddPlayerToGroup(
		conImpl->hostGroup(),
		conImpl->thisPlayer(),
		NULL,
		NULL,
		DPNADDPLAYERTOGROUP_SYNC );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	return con;
}
