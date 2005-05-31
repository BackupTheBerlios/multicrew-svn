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

#include "../multicrewgauge/gauges.h"
#include "streams.h"
#include "multicrewcore.h"
#include "fsuipcmodule.h"
#include "fsuipc.h"
#include "log.h"
#include "callback.h"
#include "packets.h"


#define WAITTIME 300


/******************** packets *******************************/
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


/*********************************************************************/
class FsuipcWatch : public Shared {
public:
	FsuipcWatch( SmartPtr<Fsuipc> fsuipc, WORD id, BYTE size, Priority prio, bool safe ) {
		this->id = id;
		this->size = size;
		this->_prio = prio;
		this->_safe = safe;
		this->data = malloc(size);
		this->oldData = malloc(size);
		this->fsuipc = fsuipc;
		ZeroMemory( this->data, size );
		ZeroMemory( this->oldData, size );
	}

	~FsuipcWatch() {
		delete data;
		delete oldData;
	}

	void update() {
		bool ok = fsuipc->read( id, size, data );
		if( !ok ) {
			dout << "FSUIPC error for id " << id << std::endl;
		}
	}

	SmartPtr<FsuipcPacket> process( bool fullSend ) {
		if( fullSend || memcmp( oldData, data, size )!=0 ) {
			// changed
			dout << "FSUIPC change for id " << id << std::endl;
			memcpy( oldData, data, size );
			return new FsuipcPacket( id, size, data );
		}
		
		return 0;
	}

	bool safe() {
		return _safe;
	}

	Priority priority() {
		return _prio;
	}

private:
	WORD id;
	BYTE size;
	Priority _prio;
	bool _safe;
	void *data;
	void *oldData;
	SmartPtr<Fsuipc> fsuipc;
};


/*********************************************************************/
struct FsuipcModule::Data {
	typedef SmartPtr<FsuipcWatch> SmartFsuipcWatch;
	std::deque<SmartFsuipcWatch> watches;
	SmartPtr<Fsuipc> fsuipc;
	SmartPtr<MulticrewCore> core;
	bool nextIsFullSend;
};


FsuipcModule::FsuipcModule() 
	: MulticrewModule( "FSUIPC" ), NetworkChannel( "FSUIPC" ) {
	d = new Data;
	d->core = MulticrewCore::multicrewCore();
	d->fsuipc = Fsuipc::fsuipc();
}


FsuipcModule::~FsuipcModule() {
	stopThread();
	d->watches.clear();
	delete d;
}


SmartPtr<PacketBase> FsuipcModule::createPacket( SharedBuffer &buffer ) {
	return new FsuipcPacket( buffer );
}


unsigned  FsuipcModule::threadProc( void *param ) {
	while( !shouldExit( WAITTIME ) ) {
		switch( d->core->mode() ) {
		case MulticrewCore::IdleMode: break;
		case MulticrewCore::HostMode:
		{
			// let watches call read
			d->fsuipc->begin();
			for( int i=0; i<d->watches.size(); i++ ) {
				d->watches[i]->update();
			}
			bool ok = d->fsuipc->end();
			
			// let watches process read results
			if( ok ) {
				// full send?
				bool fullSend = d->nextIsFullSend;
				
				// next is no full send by default
				d->nextIsFullSend = false;
				
				// get packets from watches and send
				for( i=0; i<d->watches.size(); i++ ) {
					SmartPtr<FsuipcPacket> packet = d->watches[i]->process( fullSend );
					if( !packet.isNull() ) {
						send( &*packet, d->watches[i]->safe(), d->watches[i]->priority() );
					}
				}
			}
		} break;
		case MulticrewCore::ClientMode: break;
		}
	}
		
	return 0;
}


void FsuipcModule::receive( SmartPtr<PacketBase> packet ) {
	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode:
	case MulticrewCore::ClientMode: 
	{
		SmartPtr<FsuipcPacket> pp = (FsuipcPacket*)&*packet;
		
		// write variables from the FS
		d->fsuipc->begin();
		bool ok = d->fsuipc->write( pp->id(), pp->size(), pp->data() );
		ok = ok && d->fsuipc->end();
		if( !ok ) {
			dout << "FSUIPC write error id " << pp->id() << std::endl;
		}
	} break;
	}
}


void FsuipcModule::sendFullState() {
	d->nextIsFullSend = true;
}


void FsuipcModule::watch( WORD id, BYTE size, bool safe, bool highPrio ) {
	d->watches.push_back( 
		new FsuipcWatch( d->fsuipc, id, size, 
						 highPrio?highPriority:mediumPriority, 
						 safe ));
}
