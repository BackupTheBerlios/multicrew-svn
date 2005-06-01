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

#ifndef MULTICREWCORE_H_INCLUDED
#define MULTICREWCORE_H_INCLUDED

#include "common.h"

#include <string>

#include "../stlplus/source/string_utilities.hpp"
#include "shared.h"
#include "signals.h"
#include "network.h"
#include "packets.h"
#include "config.h"
#include "thread.h"


class DLLEXPORT Identified {
 public:
	Identified( Identified *parent, std::string postfix ) 
		: _id( parent->id() + "!" + postfix ) {
	}

	Identified( std::string id ) 
		: _id( id ) {
	}

	Identified( Identified *parent, unsigned postfix ) 
		: _id( parent->id() + "!" + to_string( postfix ) ) {
	}

	std::string id() { return _id; }

 private:
	std::string _id;
};


class DLLEXPORT MulticrewModule : public Shared, public Identified {
 public:
	MulticrewModule( std::string moduleName );
	virtual ~MulticrewModule();
	
	SmartPtr<FileConfig> config();
	
 private:
	struct Data;
	friend Data;
	Data *d;
};


class DLLEXPORT AsyncCallee {
 public:
	void triggerAsyncCallback();
	virtual void asyncCallback()=0;
};


class DLLEXPORT NetworkChannel : public PacketFactory<PacketBase> {
 public:
	NetworkChannel( std::string id );
	virtual ~NetworkChannel();
	std::string channelId();
	unsigned channelNum();

	bool send( SmartPtr<PacketBase> packet, bool safe, Priority prio );
	virtual void receive( SmartPtr<PacketBase> packet )=0;
	virtual void sendFullState()=0;

 private:
	struct Data;
	friend Data;
	Data *d;
};


class DLLEXPORT MulticrewCore : public Shared {
 public:
	static SmartPtr<MulticrewCore> multicrewCore();	

	/* mode handling */
	enum Mode {
		IdleMode,
		HostMode,
		ClientMode,
	};

	virtual Mode mode()=0;
	virtual void setMode( Mode newMode )=0;
	Signal1<Mode> modeChanged;

	/* connection handling */
	virtual void useConnection( SmartPtr<Connection> con )=0;
	virtual void sendFullState()=0;
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer )=0;

	/* general stuff */
	virtual void log( std::string line )=0;
	Signal1<const char *> logged;
	virtual double time()=0;

	/* asynchronous callbacks */
	Signal initAsyncCallback;
	virtual void callbackAsync()=0;

	virtual void ackNewFrame()=0;
	Signal frameSignal;

 protected:
	friend Connection;
	virtual void receive( SmartPtr<PacketBase> packet )=0;

 protected:
	friend AsyncCallee;
	virtual void triggerAsyncCallback( AsyncCallee *callee )=0;

 private:
	friend MulticrewModule;
	virtual void registerModule( MulticrewModule *module )=0;
	virtual void unregisterModule( MulticrewModule *module )=0;   
};


#endif
