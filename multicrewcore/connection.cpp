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


// {B1449AC0-D32F-4b58-81E7-AA5F5D424CBD}
const GUID gMulticrewGuid = 
{ 0xb1449ac0, 0xd32f, 0x4b58, { 0x81, 0xe7, 0xaa, 0x5f, 0x5d, 0x42, 0x4c, 0xbd } };


class RoutedModulePacket : public Packet {
public:
	RoutedModulePacket( std::string moduleName, SmartPtr<ModulePacket> packet ) {
		this->moduleName = moduleName;
		this->packet = packet;
	}	

	RoutedModulePacket( SharedBuffer &buffer, SmartPtr<MulticrewModule> mod ) 
		: moduleName( (char*)buffer.data() ) {		
		packet = new ModulePacket( SharedBuffer(buffer,moduleName.size()+1), mod );
	}

	virtual unsigned compiledSize() {
		return moduleName.length()+1+packet->compiledSize();
	}

	virtual void compile( void *data ) {
		strcpy( (char*)data, moduleName.c_str() );
		packet->compile( ((char*)data)+moduleName.length()+1 );
	}

	std::string module() {
		return moduleName;
	}

	SmartPtr<ModulePacket> modulePacket() {
		return packet;
	}

	static std::string module( SharedBuffer &buffer ) {
		return (char*)buffer.data();
	}

private:	
	std::string moduleName;
	SmartPtr<ModulePacket> packet;
};

SmartPtr<Packet> ModulePacket::createChild( SharedBuffer &buffer ) {
	return mod->createPacket( buffer );
}

/*******************************************************************/

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

	CRITICAL_SECTION critSect;
};


ConnectionImpl::ConnectionImpl( IDirectPlay8Peer *peer ) {
	d = new Data( this );
	d->disconnected = false;
	d->peer = peer;
	d->hostGroup = 0;
	d->clientGroup = 0;
	d->thisPlayer = 0;
	d->peer->AddRef();

	InitializeCriticalSection( &d->critSect );
}


ConnectionImpl::~ConnectionImpl() {
	// shutdown DirectPlay
	if( d->peer ) d->peer->Release();

	DeleteCriticalSection( &d->critSect );
	delete d;
}


void ConnectionImpl::addModule( MulticrewModule *module ) {
	EnterCriticalSection( &d->critSect );
	modules[module->moduleName()] = module;
	LeaveCriticalSection( &d->critSect );
}


void ConnectionImpl::removeModule( MulticrewModule *module ) {
	EnterCriticalSection( &d->critSect );
	modules.erase( modules.find(module->moduleName()) );
	LeaveCriticalSection( &d->critSect );
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


PFNDPNMESSAGEHANDLER ConnectionImpl::callback() {
	return d->callbackAdapter.callback();
}


HRESULT ConnectionImpl::messageCallback( PVOID pvUserContext, DWORD dwMessageType, 
										 PVOID pMessage ) {
	//dout << "DirectPlay message of type " << dwMessageType << std::endl;
	
	switch( dwMessageType ) {
	case DPN_MSGID_RECEIVE: 
	{
		DPNMSG_RECEIVE *rec = (DPNMSG_RECEIVE*)pMessage;

		// find destination
		EnterCriticalSection( &d->critSect );
		std::map<std::string,MulticrewModule*>::iterator dest;
		SharedBuffer packetBuf(rec->pReceiveData, rec->dwReceiveDataSize, false);
		std::string moduleId = RoutedModulePacket::module( packetBuf );
		dest = modules.find( moduleId );
		if( dest==modules.end() ) {
			LeaveCriticalSection( &d->critSect );
			dout << "Unroutable packet for module \"" << moduleId << "\"" << std::endl;
			break;
		}			
		LeaveCriticalSection( &d->critSect );

		// create packet object
		SmartPtr<RoutedModulePacket> packet = 
			new RoutedModulePacket( packetBuf, (MulticrewModule*)(dest->second) );
		//dout << "Received packet " << std::endl;

		// deliver packet
		(*dest).second->receive( &*packet->modulePacket() );
	}
	break;

	case DPN_MSGID_SEND_COMPLETE:
	{
		DPNMSG_SEND_COMPLETE *msg = (DPNMSG_SEND_COMPLETE*)pMessage;

		// from Sender object?
		if( msg->pvUserContext!=0 ) {
			MulticrewModule *sender = (MulticrewModule*)msg->pvUserContext;
			//dout << "send complete message for " << msg->pvUserContext << std::endl;
			
			// call callback
			if( FAILED(msg->hResultCode) )
				sender->sendFailed();
			else
				sender->sendCompleted();

			//dout << "send complete handler returned " << msg->pvUserContext << std::endl;
		}
	}
	break;
			
	case DPN_MSGID_CREATE_GROUP: 
	{
		// group creation message, either host or client group
		DPNMSG_CREATE_GROUP *info = (DPNMSG_CREATE_GROUP*)pMessage;
		dout << "New group " << info->dpnidGroup << std::endl;
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
		dlog << "Group \"" << name << "\" created." << std::endl;
		if( strcmp(name, "Host" )==0 )
			d->hostGroup = info->dpnidGroup;
		else if( strcmp(name, "Client" )==0 ) {
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
		dlog << "New player " << player->dpnidPlayer << std::endl;

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
			dlog << "It's the host." << std::endl;
		} else {
			thisIsHost = false;
			dlog << "It's a client." << std::endl;
			group = d->clientGroup;
		}
		delete playerInfo;

		// the first is the local client
		if( d->thisPlayer==0 ) {
			dlog << "Oh, that's me." << std::endl;
			d->thisPlayer = player->dpnidPlayer;
			d->hostMode = thisIsHost;
		}
		
		// add to group
		if( group!=0 ) {
			dlog << "Adding player " << player->dpnidPlayer << " to " << group << std::endl;
			HRESULT hr = d->peer->AddPlayerToGroup(
				group,
				player->dpnidPlayer,
				NULL,
				NULL,
				DPNADDPLAYERTOGROUP_SYNC );
			if( FAILED(hr) && hr!=DPNERR_PLAYERALREADYINGROUP ) {
				dlog << "Failed: " << fe(hr).c_str() << std::endl;
				return S_OK;
			}
		}
	}
	break;

	case DPN_MSGID_ADD_PLAYER_TO_GROUP:
	{
		PDPNMSG_ADD_PLAYER_TO_GROUP msg = (PDPNMSG_ADD_PLAYER_TO_GROUP)pMessage;
		dout << "Peer " << msg->dpnidPlayer << " joining group " << msg->dpnidGroup << std::endl;
		DPNHANDLE asyncHandle;
		HRESULT hr = d->peer->AddPlayerToGroup(
			msg->dpnidGroup,
			msg->dpnidPlayer,
			NULL,
			&asyncHandle,
			0 );
		if( FAILED(hr) && hr!=DPNERR_PLAYERALREADYINGROUP ) {
			dout << "Failed: " << fe(hr).c_str() << std::endl;
			return S_OK;
		}
	}
	break;

	case DPN_MSGID_TERMINATE_SESSION:
		if( !d->disconnected ) {
			dlog << "Session terminated" << std::endl;
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
		d->disconnected = true;
	
		ref();
		dlog << "Terminating session" << std::endl;
		d->peer->TerminateSession( NULL, 0, 0 );
		d->peer->CancelAsyncOperation( 0, DPNCANCEL_ALL_OPERATIONS );
		
		dlog << "Closing peer" << std::endl;
		d->peer->Close( 0 );
		disconnected.emit();
		deref();
	}
}


bool ConnectionImpl::start() {
	return true;
}


bool ConnectionImpl::send( SmartPtr<ModulePacket> packet, bool safe, 
						   Priority prio, SmartPtr<MulticrewModule> sender,
						   bool callback ) {
	// connected?
	if( d->peer==0 || d->disconnected ) {
		if( !sender.isNull() ) sender->sendFailed();
		return false;
	}

	//dout << "sending to " << (d->hostMode?d->clientGroup:d->hostGroup) << std::endl;

	// prepare packet
	SmartPtr<RoutedModulePacket> routed = 
		new RoutedModulePacket( sender->moduleName(), packet );
	unsigned size = routed->compiledSize();
	void *buffer = malloc( size );
	routed->compile( buffer );

	// send packet
	DPN_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(DPN_BUFFER_DESC) );
	desc.pBufferData = (BYTE*)buffer;
	desc.dwBufferSize = size;
	DPNHANDLE asyncHandle;
	HRESULT hr = d->peer->SendTo(
		DPNID_ALL_PLAYERS_GROUP, //d->hostMode?d->clientGroup:d->hostGroup,
		&desc,
		1,
		0,
		callback?(&*sender):0,
		&asyncHandle,
		((prio==highPriority)?DPNSEND_PRIORITY_HIGH:0) |
		((prio==lowPriority)?DPNSEND_PRIORITY_LOW:0) |
		(safe?DPNSEND_GUARANTEED:0) |
		DPNSEND_NOLOOPBACK | DPNSEND_COALESCE );

	// free packet buffer
	free( buffer );

	// async operation?
	if( hr==DPNSUCCESS_PENDING ) {
		//dout << "async" << std::endl;
		return true;
	}

	// not async -> remove callback
	if( callback!=0 ) {
		//dout << "sync" << std::endl;
	}

    // error?
	if( FAILED(hr) ) {
		if( !sender.isNull() ) sender->sendFailed();
		dout << "Failed to send packet: " << fe(hr).c_str() << std::endl;
		return false;
	}

	// no error -> callback?
	if( !sender.isNull() ) {
		//dout << "completed" << std::endl;
		sender->sendCompleted();
	}
	
	return true;
}
