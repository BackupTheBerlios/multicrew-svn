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

#pragma warning (disable : 4786)

#include "common.h"

#include <deque>
#include <map>
#include <set>

#include "../stlplus/source/string_utilities.hpp"

#include "common.h"
#include "streams.h"
#include "signals.h"
#include "log.h"


#define RESERVED_CHANNELS 16

enum {
	fullSendPacket=0,
	channelPacket=1,
	idChannelPacket=2,
	idNumPacket=3,
};


typedef TypedPacket<unsigned __int16, PacketBase> MulticrewPacket;
typedef EmptyPacket FullSendPacket;
typedef StringPacket IdNumPacket;


class IdChannelPacket : public StringTypedPacket<PacketBase> {
 public:
	IdChannelPacket( std::string id, SmartPtr<PacketBase> packet ) 
		: StringTypedPacket<PacketBase>( id, packet ) {
	}	

	IdChannelPacket( SharedBuffer &buffer, Factory *factory ) 
		: StringTypedPacket<PacketBase>( buffer, factory ) {
	}

	std::string id() {
		return key();
	}
};


typedef std::set<NetworkChannel*> NetworkChannelSet;


/*********************************************************************/


struct NetworkChannel::Data {
	SmartPtr<MulticrewCore> core;
	std::string id;
	unsigned num;	
};


NetworkChannel::NetworkChannel( std::string id ) {
	d = new Data;
	d->core = (MulticrewCore*)&*MulticrewCore::multicrewCore();
	d->id = id;
	d->num = d->core->registerNetworkChannel( this );
}


NetworkChannel::~NetworkChannel() {
	d->core->unregisterNetworkChannel( this );
	delete d;
}


std::string NetworkChannel::channelId() {
	return d->id;
}


unsigned NetworkChannel::channelNum() {
	return d->num;
}


void NetworkChannel::setChannelNum( unsigned num ) {
	d->num = num;
}


bool NetworkChannel::send( SmartPtr<PacketBase> packet, bool safe, 
						   Priority prio ) {
	return d->core->sendChannelPacket(
		packet,	safe, prio, this );
}


/*********************************************************************/


typedef std::deque<SharedBuffer> SharedBufferDeque;


struct ChannelProtocol::Data {
	Mode mode;
	CRITICAL_SECTION critSect;

	/* network stuff */
	SmartPtr<Connection> connection;
	std::map<std::string, NetworkChannelSet> channels; 
	std::map<unsigned, std::string> numToId;
	std::map<std::string, unsigned> idToNum;
	std::map<unsigned, SharedBufferDeque> todo;
	unsigned sentReservedChannelPackets;
};


ChannelProtocol::ChannelProtocol() {
	d = new Data;
	d->mode = IdleMode;
	d->sentReservedChannelPackets = 0;
	InitializeCriticalSection( &d->critSect );
}


ChannelProtocol::~ChannelProtocol() {
	DeleteCriticalSection( &d->critSect );

	delete d;
}


ChannelProtocol::Mode ChannelProtocol::mode() {
	return d->mode;
}


void ChannelProtocol::start( bool host, SmartPtr<Connection> con ) {
	EnterCriticalSection( &d->critSect );
	d->mode = host?HostMode:ClientMode;
	d->connection = con;
	d->idToNum.clear();
	d->numToId.clear();
	d->todo.clear();
	LeaveCriticalSection( &d->critSect );

	// startup procedure
	sendFullState();
	modeChanged.emit( d->mode );
}


void ChannelProtocol::stop() {
	EnterCriticalSection( &d->critSect );
	d->mode = IdleMode;
	d->connection = 0;
	LeaveCriticalSection( &d->critSect );

	modeChanged.emit( d->mode );
}


unsigned ChannelProtocol::registerNetworkChannel( NetworkChannel *channel ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string, NetworkChannelSet>::iterator it =
		d->channels.find( channel->channelId() );
	if( it==d->channels.end() )
		d->channels[channel->channelId()] = NetworkChannelSet();
	d->channels[channel->channelId()].insert( channel );
	unsigned ret = getIdNum( channel->channelId() );
	//dout << "Network channel " << channel->channelId() << " max=" << ret << std::endl;
	LeaveCriticalSection( &d->critSect );
	return ret;
}


void ChannelProtocol::unregisterNetworkChannel( NetworkChannel *channel ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannelSet>::iterator it =
		d->channels.find( channel->channelId() );
	if( it!=d->channels.end() ) 
		d->channels[channel->channelId()].erase( channel );
	LeaveCriticalSection( &d->critSect );
}


unsigned ChannelProtocol::reservedChannel() {
	return (d->sentReservedChannelPackets++) % RESERVED_CHANNELS;
}


unsigned ChannelProtocol::getIdNum( std::string id ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string, unsigned>::iterator it
		= d->idToNum.find( id );
	unsigned num=0;
	if( it!=d->idToNum.end() )
		num = it->second;
	LeaveCriticalSection( &d->critSect );
	return num;
}


std::string ChannelProtocol::getNumId( unsigned num ) {
	EnterCriticalSection( &d->critSect );
	std::map<unsigned, std::string>::iterator it
		= d->numToId.find( num );
	std::string id;
	if( it!=d->numToId.end() )
		id = it->second;
	LeaveCriticalSection( &d->critSect );
	return id;
}
	

unsigned ChannelProtocol::createIdNum( std::string id ) {
	EnterCriticalSection( &d->critSect );
	std::map<std::string, unsigned>::iterator it
		= d->idToNum.find( id );
	unsigned num = 0;
	if( it==d->idToNum.end() ) {
		num = d->idToNum.size()+RESERVED_CHANNELS;
		setIdNum( id, num );
	} else
		num = it->second;
	LeaveCriticalSection( &d->critSect );
	return num;
}


void ChannelProtocol::setIdNum( std::string id, unsigned num ) {
	//dout << "SetIdNum " << id << "=" << num << std::endl;
	EnterCriticalSection( &d->critSect );
	std::map<std::string, unsigned>::iterator it
		= d->idToNum.find( id );
	if( it==d->idToNum.end() ) {
		d->idToNum[id] = num;
		d->numToId[num] = id;
		std::map<std::string, NetworkChannelSet>::iterator it
			= d->channels.find( id );
		if( it!=d->channels.end() ) {
			NetworkChannelSet::iterator it2 = it->second.begin();
			while( it2!=it->second.end() ) {
				(*it2)->setChannelNum( num );
				it2++;
			}
		}
	}
	LeaveCriticalSection( &d->critSect );
}


bool ChannelProtocol::sendChannelPacket( SmartPtr<PacketBase> packet,										   
										   bool safe, Priority prio, 
										   NetworkChannel *channel ) {
	bool ret = false;
	EnterCriticalSection( &d->critSect );
	SmartPtr<Connection> con = d->connection;
	LeaveCriticalSection( &d->critSect );

	if( !con.isNull() ) {
		switch( mode() ) {
		case IdleMode: break;
		case HostMode: {
			// assigned num to id?
			EnterCriticalSection( &d->critSect );
			std::string id = channel->channelId();
			unsigned num = getIdNum( id );
			if( num==0 ) {
				// create new channel number
				// and send packet with the channel and the id
				num = createIdNum( id );
				LeaveCriticalSection( &d->critSect );
				//dout << "sendChannelPacket idChannelPacket to " << id << "=" << num << std::endl;
				return con->send( 
					new MulticrewPacket( 
						idChannelPacket + (num << 4), 
						new IdChannelPacket( id, packet ) ),
					true, prio, reservedChannel() );
			} else {
				LeaveCriticalSection( &d->critSect );
				//dout << "sendChannelPacket channelPacket to " << id << "=" << num << std::endl;
				return con->send( 
					new MulticrewPacket( 
						channelPacket + (num << 4), 
						packet ),
					safe, prio, num );
			}
		} break;
		case ClientMode: {
			// assigned num to id?
			EnterCriticalSection( &d->critSect );
			std::string id = channel->channelId();
			unsigned num = getIdNum( id );
			if( num==0 ) {
				// send packet with channel 0, host will reply with
				// IdNumPacket
				LeaveCriticalSection( &d->critSect );
				//dout << "sendChannelPacket idChannelPacket to " << id << "=" << num << std::endl;
				return con->send( 
					new MulticrewPacket( 
						idChannelPacket, 
						new IdChannelPacket( id, packet ) ),
					true, prio, reservedChannel() );
			} else {
				LeaveCriticalSection( &d->critSect );
				//dout << "sendChannelPacket channelPacket to " << id << "=" << num << std::endl;
				return con->send( 
					new MulticrewPacket( 
						channelPacket + (num << 4), 
						packet ),
					safe, prio, num );
			}
		} break;
		}
	}
	return false;
}


void ChannelProtocol::handleFullState() {
	EnterCriticalSection( &d->critSect );
	std::map<std::string,NetworkChannelSet>::iterator it = d->channels.begin();
	while( it!=d->channels.end() ) {
		NetworkChannelSet::iterator it2 = it->second.begin();
		while( it2!=it->second.end() ) {
			(*it2)->sendFullState();
			it2++;
		}
		it++;
	}
	LeaveCriticalSection( &d->critSect );
}


void ChannelProtocol::sendFullState() {
	switch( mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
		handleFullState();
		break;
	case MulticrewCore::ClientMode: {		
		SmartPtr<PacketBase> packet = 
			new MulticrewPacket( 
				fullSendPacket, 
				new FullSendPacket );
		d->connection->send( packet, true, highPriority, reservedChannel() );
	} break;
	}
}


SmartPtr<PacketBase> ChannelProtocol::createPacket( std::string channel, 
													  SharedBuffer &buffer ) {
    // find destination
	std::map<std::string,NetworkChannelSet>::iterator dest =
		d->channels.find( channel );
	if( dest!=d->channels.end() ) {
		std::set<NetworkChannel*>::iterator it = dest->second.begin();
		if( it!=dest->second.end() )
			return  (*it)->createPacket( buffer );
	}

	return 0;
}


void ChannelProtocol::receiveChannelPacket( std::string channel, 
											  SharedBuffer &buffer ) {
    // find destination
	std::map<std::string,NetworkChannelSet>::iterator dest =
		d->channels.find( channel );
	if( dest!=d->channels.end() ) {
		std::set<NetworkChannel*>::iterator it = dest->second.begin();
		if( it!=dest->second.end() ) {
			SmartPtr<PacketBase> packet = (*it)->createPacket( buffer );
			while( it!=dest->second.end() ) {
				(*it)->receive( packet );			
				it++;
			}
		}		
	}
}


void ChannelProtocol::receiveChannelPacket( std::string channel, 
											  SmartPtr<PacketBase> packet ) {
    // find destination
	std::map<std::string,NetworkChannelSet>::iterator dest =
		d->channels.find( channel );
	if( dest!=d->channels.end() ) {
		std::set<NetworkChannel*>::iterator it = dest->second.begin();
		while( it!=dest->second.end() ) {
			(*it)->receive( packet );			
			it++;
		}		
	}
}


void ChannelProtocol::receive( SharedBuffer &buf ) {
	unsigned key = MulticrewPacket::key( buf );
	unsigned type = key & 15; // only lower 4 bits are the packet type	
	unsigned num = key >> 4;
	switch( type ) {
	case idNumPacket: {
		SmartPtr<IdNumPacket> inp = new IdNumPacket( MulticrewPacket::data( buf ) );
		switch( mode() ) {
		case IdleMode: break;
		case ClientMode: {
			setIdNum( inp->string, num );			
			//dout << "receive idNumPacket " << inp->string << "=" << num << std::endl;
			std::map<unsigned,SharedBufferDeque>::iterator todos =
				d->todo.find( num );
			if( todos!=d->todo.end() ) {
				//dout << "receive found todos" << std::endl;
				SharedBufferDeque::iterator it = todos->second.begin();
				while( it!=todos->second.end() ) {
					receiveChannelPacket( inp->string, *it );
					it++;
				}
				todos->second.clear();
				d->todo.erase( todos );
			}
		} break;
		case HostMode: {
			// idNumPacket in HostMode means a request for a channel id
			// host replies with another idNumPacket with the set
			// channel id
			std::string id = getNumId( num );
			//dout << "receive idNumPacket " << id << "=" << num << std::endl;
			d->connection->send( 
				new MulticrewPacket( 
					idNumPacket + (num << 4), 
					new IdNumPacket( id )),
				true, highPriority, reservedChannel() );						
		} break;
		}
	} break;
	case fullSendPacket:
		handleFullState();
		break;
	case channelPacket: {
		switch( mode() ) {
		case IdleMode: break;
		case ClientMode: {
			EnterCriticalSection( &d->critSect );
			std::string id = getNumId( num );
			if( id.length()==0 ) {
				std::map<unsigned, SharedBufferDeque>::iterator it
					= d->todo.find( num );
				if( it==d->todo.end() )	d->todo[num] = SharedBufferDeque();				
				d->todo[num].push_back( MulticrewPacket::data( buf ) );
				LeaveCriticalSection( &d->critSect );
				//dout << "receive channelPacket " << num << ", sending idNumPacket" << std::endl;
				d->connection->send( 
					new MulticrewPacket( 
						idNumPacket + (num << 4),
						new IdNumPacket( "" )),
					true, highPriority, reservedChannel() );				
			} else {
				LeaveCriticalSection( &d->critSect );
				//dout << "receive channelPacket " << id << "=" << num << std::endl;
				receiveChannelPacket( id, MulticrewPacket::data( buf ) );
			}
		} break;
		case HostMode: {			
			std::string id = getNumId( num );
			//dout << "receive channelPacket " << id << "=" << num << std::endl;
			receiveChannelPacket( id, MulticrewPacket::data( buf ) );			
		} break;
		}
	} break;
	case idChannelPacket: {
		switch( mode() ) {
		case IdleMode: break;
		case ClientMode: {
			EnterCriticalSection( &d->critSect );
			SmartPtr<IdChannelPacket> icp = 
				new IdChannelPacket( MulticrewPacket::data( buf ), this );
			setIdNum( icp->key(), num );			
			//dout << "receive idChannelPacket " << icp->key() << "=" << num << std::endl;
			LeaveCriticalSection( &d->critSect );
			receiveChannelPacket( icp->key(), icp->wrappee() );			
		} break;
		case HostMode: {
			EnterCriticalSection( &d->critSect );
			SmartPtr<IdChannelPacket> icp = 
				new IdChannelPacket( MulticrewPacket::data( buf ), this );
			receiveChannelPacket( icp->key(), icp->wrappee() );
			LeaveCriticalSection( &d->critSect );
			unsigned num = createIdNum( icp->key() );
			//dout << "receive idChannelPacket " << icp->key() << "=" << num << ", sending idNumPacket" << std::endl;
			d->connection->send( 
				new MulticrewPacket( 
					idNumPacket + (num << 4), 
					new IdNumPacket( icp->key() )),
				true, highPriority, reservedChannel() );
		} break;
		}
	} break;
	default:
		dout << "Unknown packet type " << type << std::endl;
		break;
	}
}

