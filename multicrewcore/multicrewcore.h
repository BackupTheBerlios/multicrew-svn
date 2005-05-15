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

#include "shared.h"
#include "signals.h"
#include "network.h"
#include "packets.h"
#include "config.h"
#include "thread.h"

class MultisyncCore;
class DLLEXPORT MulticrewModule : 
    public ModulePacketFactory,
	public Shared, 
	public Thread {
public:
	MulticrewModule( std::string moduleName, unsigned minSendWait=-1 );
	virtual ~MulticrewModule();

	std::string moduleName();
	SmartPtr<FileConfig> config();

	/* network */
	virtual void sendCompleted();
	virtual void sendFailed();
	virtual void receive( SmartPtr<ModulePacket> packet );
	SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );
	virtual void sendFullState()=0;
	void requestFullState();
	
protected:
	friend class TypedInnerModulePacketFactory;

	/* network */
	virtual void handlePacket( SmartPtr<PacketBase> packet )=0;
	void lock();
	void send( SmartPtr<PacketBase> packet, bool safe, Connection::Priority prio );
	bool sendAsync( SmartPtr<PacketBase> packet, bool safe, Connection::Priority prio, int channel=1 );
	void unlock();

	virtual void sendProc();
	virtual SmartPtr<PacketBase> createInnerModulePacket( SharedBuffer &buffer )=0;
	void disconnect();

private:
	friend class MulticrewCore;

	void connect( SmartPtr<Connection> con );
	virtual unsigned threadProc( void *param );

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


class DLLEXPORT MulticrewCore : public Shared {
public:
	static SmartPtr<MulticrewCore> multicrewCore();	

	/* mode handling */
	enum Mode {
		IdleMode,
		HostMode,
		ClientMode,
	};
	Mode mode();
	void setMode( Mode newMode );
	Signal1<Mode> modeChanged;

	/* connection handling */
	void prepare( SmartPtr<Connection> con );
	void unprepare();

	/* general stuff */
	void log( std::string line );
	Signal1<const char *> logged;
	double time();

	/* asynchronous callbacks */
	Signal initAsyncCallback;
	void callbackAsync();

	void ackNewFrame();
	Signal frameSignal;

 protected:
	friend AsyncCallee;
	void triggerAsyncCallback( AsyncCallee *callee );

 private:
	friend MulticrewModule;
	void registerModule( MulticrewModule *module );
	void unregisterModule( MulticrewModule *module );   
	
 private:
	MulticrewCore();
	virtual ~MulticrewCore();
	void start();

	struct Data;
	Data *d;
};


#endif
