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


#if defined (_DEBUG)
const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP = 
{ 0x8d3f9e5e, 0xa3bd, 0x475b, 0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c };
#endif


class HostConnectionImpl : public HostConnection {
public:
	HostConnectionImpl( IDirectPlay8Peer *peer );
	virtual ~HostConnectionImpl();

	virtual void disconnect();
	virtual void start();
	virtual bool send( Packet *packet, bool safe, bool sync=false );

	PFNDPNMESSAGEHANDLER callback();
	DPNID hostGroup();
	DPNID clientGroup();
	DPNID hostPlayer();

private:
	HRESULT callback( PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage );

	struct Data;
	friend Data;
	Data *d;
};

struct HostConnectionImpl::Data {
	Data( HostConnectionImpl *con, IDirectPlay8Peer *peer ) 
		: callbackAdapter( con, HostConnectionImpl::callback ) {
		this->peer = peer;
		hostGroup = 0;
		clientGroup = 0;
		hostPlayer = 0;
		peer->AddRef();
		disconnecting = false;
	}
	virtual ~Data() {
		peer->Release();
	}

	bool disconnecting;
	DPNID hostGroup;
	DPNID clientGroup;
	DPNID hostPlayer;
	IDirectPlay8Peer *peer;
	CallbackAdapter3<HRESULT, HostConnectionImpl, PVOID, DWORD, PVOID> callbackAdapter;
};

HostConnectionImpl::HostConnectionImpl( IDirectPlay8Peer *peer ) {
	d = new Data( this, peer );
}

HostConnectionImpl::~HostConnectionImpl() {
	delete d;
}

PFNDPNMESSAGEHANDLER HostConnectionImpl::callback() {
	return d->callbackAdapter.callback();
}

DPNID HostConnectionImpl::hostGroup() { 
	return d->hostGroup; 
}

DPNID HostConnectionImpl::clientGroup() { 
	return d->clientGroup; 
}

DPNID HostConnectionImpl::hostPlayer() { 
	return d->hostPlayer; 
}

HRESULT HostConnectionImpl::callback( PVOID pvUserContext, DWORD dwMessageType, 
									  PVOID pMessage ){
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

	case DPN_MSGID_CREATE_GROUP: {
		// group creation message, either host or client group
		DPNMSG_CREATE_GROUP *info = (DPNMSG_CREATE_GROUP*)pMessage;
		log << "Getting group info for " << info->dpnidGroup << std::endl;
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
		log << "Group \"" << name << "\" created." << std::endl;
		if( strcmp(name, "Host" ) )
			d->hostGroup = info->dpnidGroup;
		else if( strcmp(name, "Client" ) )
			d->clientGroup = info->dpnidGroup;

		// delete buffer
		delete groupInfo;
	}
		break;
		
			
	case DPN_MSGID_CREATE_PLAYER: 
	{
		// player found, the first is the host
		DPNMSG_CREATE_PLAYER *player = (DPNMSG_CREATE_PLAYER*)pMessage;
		log << "New player " << player->dpnidPlayer << std::endl;
		if( d->hostPlayer==0 ) {
			log << "Oh, that's me." << std::endl;
			d->hostPlayer = player->dpnidPlayer;
		}
	}
	break;

	case DPN_MSGID_TERMINATE_SESSION:
		if( !d->disconnecting ) {
			log << "Session terminated" << std::endl;
			ref();
			disconnected.emit();
			d->peer->Close( 0 );
			deref();
		}
		break;

	};
	return S_OK;
}

void HostConnectionImpl::disconnect() {
	d->disconnecting = true;
	
	ref();
	log << "Terminating session" << std::endl;
	d->peer->TerminateSession( NULL, 0, 0 );

	log << "Closing peer" << std::endl;
	d->peer->Close( 0 );
 	disconnected.emit();
	deref();
}

void HostConnectionImpl::start() {
}

bool HostConnectionImpl::send( Packet *packet, bool safe, bool sync ) {
	if( !d->peer ) return false;

	//dout << "Sending packet of type " << packet->id << std::endl;

	DPNHANDLE asyncHandle;
	DPN_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(DPN_BUFFER_DESC) );
	desc.pBufferData = (BYTE*)packet;
	desc.dwBufferSize = packet->size;
	HRESULT hr = d->peer->SendTo(
		d->clientGroup, //DPNID_ALL_PLAYERS_GROUP,
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
	if( d->deviceAddress )
		d->deviceAddress->Release();

	if( d->peer ) {
		d->peer->Release();
	}
	delete d;
}

SmartPtr<HostConnection> HostConnectionSetup::host( int port, std::wstring sessionName, 
												    bool passwordEnabled, std::wstring password  ) {
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
	SmartPtr<HostConnection> con( conImpl );

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
#if defined (_DEBUG)
	hr = d->deviceAddress->SetSP( &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP );
	if( FAILED(hr) ) {
#endif
		hr = d->deviceAddress->SetSP( &CLSID_DP8SP_TCPIP );
		if( FAILED(hr) ) {
			log << "Failed: " << fe(hr).c_str() << std::endl;
			return 0;
		}
#if defined (_DEBUG)
		else {
			dout << "Using network simulator"  std::endl;
		}
	}
#endif

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
	wchar_t name[16];
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
		conImpl->hostPlayer(),
		NULL,
		NULL,
		DPNADDPLAYERTOGROUP_SYNC );
	if( FAILED(hr) ) {
		log << "Failed: " << fe(hr).c_str() << std::endl;
		return false;
	}

	return con;
}
