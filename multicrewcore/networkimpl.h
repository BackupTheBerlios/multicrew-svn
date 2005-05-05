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

#ifndef MULTICREWCORE_NETWORKIMPL_H_INCLUDED
#define MULTICREWCORE_NETWORKIMPL_H_INCLUDED

#include "common.h"

#include <windows.h>
#include <NetworkTypes.h>
#include "PacketPriority.h"

#include "network.h"


class ConnectionImpl : public Connection, 
					   public Thread {
public:
	ConnectionImpl();
	virtual ~ConnectionImpl();

	virtual void addModule( MulticrewModule *module );
	virtual void removeModule( MulticrewModule *module );

	virtual bool send( SmartPtr<ModulePacket> packet, bool safe, 
					   Priority prio, SmartPtr<MulticrewModule> sender,
					   bool async=false, int channel=0 );
	virtual void disconnect();
	virtual bool start();

protected:
	std::map<std::string,MulticrewModule*> modules;

	virtual bool sendImpl( char *buf, unsigned len, 
						   PacketPriority priority, 
						   PacketReliability reliability, 
						   char orderingChannel )=0;
	virtual void disconnectImpl()=0;
	virtual void processImpl()=0;
	//virtual bool connectedImpl()=0;
	virtual unsigned threadProc( void *param );
	void processPacket( void *data, unsigned length );

	struct Data;
	friend Data;
	Data *d;
};


#endif
