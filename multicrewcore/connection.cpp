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
#include "networkimpl.h"
#include "callback.h"


// {B1449AC0-D32F-4b58-81E7-AA5F5D424CBD}
const GUID gMulticrewGuid = 
{ 0xb1449ac0, 0xd32f, 0x4b58, { 0x81, 0xe7, 0xaa, 0x5f, 0x5d, 0x42, 0x4c, 0xbd } };


struct ConnectionImpl::Data {
	Data( ConnectionImpl *con ) 
		: callbackAdapter( con, ConnectionImpl::messageCallback ) {
	}

	bool disconnected;
	IDirectPlay8Peer *peer;
	bool hostMode;
	DPNID clientGroup;
	DPNID hostGroup;
	DPNID thisPlayer;
	CallbackAdapter3<HRESULT, class ConnectionImpl, PVOID, DWORD, PVOID> callbackAdapter;
};


ConnectionImpl::ConnectionImpl( IDirectPlay8Peer *peer ) {
	d = new Data( this );
	d->disconnected = false;

	d->peer = peer;
	d->hostGroup = 0;
	d->clientGroup = 0;
	d->thisPlayer = 0;
	d->peer->AddRef();
}


ConnectionImpl::~ConnectionImpl() {
	if( d->peer ) d->peer->Release();
	delete d;
}


void ConnectionImpl::addReceiver( Receiver *receiver ) {
	receivers[receiver->id()] = receiver;
}


void ConnectionImpl::removeReceiver( Receiver *receiver ) {
	receivers.erase( receivers.find(receiver->id()) );
}


IDirectPlay8Peer *ConnectionImpl::peer() {
	return d->peer;
}


DPNID ConnectionImpl::clientGroup() {
	return d->clientGroup;
}


DPNID ConnectionImpl::hostGroup() {
	return d->hostGroup;
}


DPNID ConnectionImpl::thisPlayer() {
	return d->thisPlayer;
}


void ConnectionImpl::deliverModulePacket( ModulePacket *packet ) {
	// find destination
	std::map<std::string,Receiver*>::iterator dest;
	dest = receivers.find(packet->module);
	if( dest==receivers.end() ) {
		dout << "Unroutable packet for module \"" << packet->module << "\"" << std::endl;
		return;
	}

	// deliver packet
	(*dest).second->receive(packet);
}


PFNDPNMESSAGEHANDLER ConnectionImpl::callback() {
	return d->callbackAdapter.callback();
}


HRESULT ConnectionImpl::messageCallback( PVOID pvUserContext, DWORD dwMessageType, 
								  PVOID pMessage ) {
//	dout << "DirectPlay message of type " << dwMessageType << std::endl;
	
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
			dout << "Failed: " << fe(hr).c_str() << std::endl;
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
		}
		
		// delete buffer
		delete groupInfo;
	}
	break;
		
	case DPN_MSGID_CREATE_PLAYER: 
	{
		// player found
		DPNMSG_CREATE_PLAYER *player = (DPNMSG_CREATE_PLAYER*)pMessage;
		dout << "New player " << player->dpnidPlayer << std::endl;

		// get player info
		dout << "Get player info" << std::endl;
		DPN_PLAYER_INFO *playerInfo = new DPN_PLAYER_INFO;
		playerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
		DWORD size = sizeof(DPN_PLAYER_INFO);
		HRESULT hr = d->peer->GetPeerInfo( 
			player->dpnidPlayer,
			playerInfo,
			&size,
			0);
		if( hr==DPNERR_BUFFERTOOSMALL ) {
			delete playerInfo;
			playerInfo = (DPN_PLAYER_INFO *)new char[size];
			playerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
			hr = d->peer->GetPeerInfo( 
				player->dpnidPlayer,
				playerInfo,
				&size,
				0);
		}
		if( FAILED(hr) ) {
			dout << "Failed: " << fe(hr).c_str() << std::endl;
			delete playerInfo;
			return S_OK;
		}

		// which group?
		DPNID group;
		char name[1024];
		bool thisIsHost;
		wcstombs( name, playerInfo->pwszName, 1024 );
		if( strcmp(name, "Host")==0 ) {
			thisIsHost = true;
			group = d->hostGroup;
			dout << "It's the host." << std::endl;
		} else {
			thisIsHost = false;
			dout << "It's a client." << std::endl;
			group = d->clientGroup;
		}
		delete playerInfo;

		// the first is the local client
		if( d->thisPlayer==0 ) {
			dout << "Oh, that's me." << std::endl;
			d->thisPlayer = player->dpnidPlayer;
			d->hostMode = thisIsHost;
		}
		
		// add to group
		if( group!=0 ) {
			dout << "Adding player to group." << std::endl;
			HRESULT hr = d->peer->AddPlayerToGroup(
				group,
				player->dpnidPlayer,
				NULL,
				NULL,
				DPNADDPLAYERTOGROUP_SYNC );
			if( FAILED(hr) && hr!=DPNERR_PLAYERALREADYINGROUP ) {
				dout << "Failed: " << fe(hr).c_str() << std::endl;
				return false;
			}
		}
	}
	break;

	case DPN_MSGID_TERMINATE_SESSION:
		if( !d->disconnected ) {
			dout << "Session terminated" << std::endl;
			ref();
			d->peer->Close( 0 );
			disconnected.emit();
			deref();
		}
		break;
	
	};

	return S_OK;
}


void ConnectionImpl::disconnect() {
	if( !d->disconnected ) {
		log << "Closing peer" << std::endl;
		d->disconnected = true;
	
		ref();
		log << "Terminating session" << std::endl;
		d->peer->TerminateSession( NULL, 0, 0 );
		
		log << "Closing peer" << std::endl;
		d->peer->Close( 0 );
		disconnected.emit();
		deref();
	}
}


void ConnectionImpl::start() {
}


bool ConnectionImpl::send( Packet *packet, bool safe, bool sync ) {
	if( !d->peer ) return false;
	
	DPNHANDLE asyncHandle;
	DPN_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(DPN_BUFFER_DESC) );
	desc.pBufferData = (BYTE*)packet;
	desc.dwBufferSize = packet->size;
	HRESULT hr = d->peer->SendTo(
		d->hostMode?d->clientGroup:d->hostGroup,
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
