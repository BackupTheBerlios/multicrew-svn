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
#include <dplay8.h>

#include "network.h"

extern const GUID gMulticrewGuid;


class ConnectionImpl : public Connection {
public:
	ConnectionImpl( IDirectPlay8Peer *peer );
	virtual ~ConnectionImpl();

	virtual void addModule( MulticrewModule *module );
	virtual void removeModule( MulticrewModule *module );

	virtual bool send( SmartPtr<ModulePacket> packet, bool safe, 
					   Priority prio, SmartPtr<MulticrewModule> sender,
					   bool callback=true );
	virtual void disconnect();
	virtual bool start();

	PFNDPNMESSAGEHANDLER callback();
	DPNID clientGroup();
	DPNID hostGroup();
	DPNID thisPlayer();

protected:
	std::map<std::string,MulticrewModule*> modules;

	DWORD throttleThreadProc( LPVOID param );
	virtual HRESULT messageCallback( PVOID pvUserContext, DWORD dwMessageType, 
									 PVOID pMessage );
	IDirectPlay8Peer *peer();

	struct Data;
	friend Data;
	Data *d;
};


#endif
