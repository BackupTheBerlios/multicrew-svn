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

#include "common.h"

#include <deque>
#include <windows.h>

#include "fsuipc/FSUIPC_User.h"
#include "../multicrewgauge/gauges.h"

#include "streams.h"
#include "fsuipcmodule.h"
#include "log.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 300


struct FsuipcStruct {
	FsuipcStruct( WORD id, BYTE size ) {
		this->id = id;
		this->size = size;
	}
	WORD id;
	BYTE size;
};


class FsuipcPacket : public WrappedPacket<FsuipcStruct, RawPacket> {
public:
	FsuipcPacket( WORD id, BYTE size, void *data )
		: WrappedPacket<FsuipcStruct, RawPacket>(
			FsuipcStruct( id, size ), new RawPacket(data,size) ) {
	}

	FsuipcPacket( SharedBuffer &buf ) 
		: WrappedPacket<FsuipcStruct, RawPacket>( buf ) {
	}

	WORD id() {
		return prefix()->id;
	}

	BYTE size() {
		return prefix()->size;
	}

	void *data() {
		return wrappee()->buffer().data();
	}

protected:
	virtual SmartPtr<RawPacket> createWrappee( SharedBuffer &buffer ) {
		return new RawPacket( buffer );
	}
};


class FsuipcWatch : public Shared {
public:
	FsuipcWatch( WORD id, BYTE size ) {
		this->id = id;
		this->size = size;
		this->data = malloc(size);
		this->oldData = malloc(size);
		ZeroMemory( this->data, size );
		ZeroMemory( this->oldData, size );
	}

	~FsuipcWatch() {
		delete data;
		delete oldData;
	}

	SmartPtr<FsuipcPacket> update() {
		DWORD res;
		bool ok = FSUIPC_Read( id, size, data, &res );
		ok = ok && FSUIPC_Process( &res );	
		if( !ok ) {
			dout << "FSUIPC error for id " << id << std::endl;
		} else {
			if( memcmp( oldData, data, size )!=0 ) {
				// changed
				dout << "FSUIPC change for id " << id << std::endl;
				memcpy( oldData, data, size );
				return new FsuipcPacket( id, size, data );
			}
		}
		
		return 0;
	}

private:
	WORD id;
	BYTE size;
	void *data;
	void *oldData;
};


struct FsuipcModule::Data {
	typedef SmartPtr<FsuipcWatch> SmartFsuipcWatch;
	std::deque<SmartFsuipcWatch> watches;
};


FsuipcModule::FsuipcModule( bool hostMode ) 
	: MulticrewModule( "FSUIPC", hostMode, hostMode?WAITTIME:-1 ) {
	d = new Data;

	MulticrewCore::multicrewCore()->registerModule( this );

	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) )
		dlog << "Cannot connect to FSUIPC" << std::endl;
}


FsuipcModule::~FsuipcModule() {
	MulticrewCore::multicrewCore()->unregisterModule( this );

	// disconnect from FSUIPC
	FSUIPC_Close();

	d->watches.clear();
	delete d;
}


SmartPtr<Packet> FsuipcModule::createPacket( SharedBuffer &buffer ) {
	return new FsuipcPacket( buffer );
}


void FsuipcModule::sendProc() {
	for( int i=0; i<d->watches.size(); i++ ) {
		SmartPtr<FsuipcPacket> packet = d->watches[i]->update();
		if( !packet.isNull() ) {
			send( &*packet, true, Connection::HighPriority );
		}
	}
}


void FsuipcModule::handlePacket( SmartPtr<Packet> packet ) {
	SmartPtr<FsuipcPacket> pp = (FsuipcPacket*)&*packet;
	
	// write variables from the FS
	DWORD res;
	bool ok = true;
	ok = FSUIPC_Write( pp->id(), pp->size(), pp->data(), &res );
	ok = ok && FSUIPC_Process( &res );
	if( !ok ) {
		dout << "FSUIPC write error id " << pp->id() << std::endl;
	}
}


void FsuipcModule::watch( WORD id, BYTE size ) {
	d->watches.push_back( new FsuipcWatch( id, size ) );
}
