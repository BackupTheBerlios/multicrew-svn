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

#include <windows.h>

#include "fsuipc/FSUIPC_User.h"
#include "../multicrewgauge/gauges.h"

#include "fsuipc.h"
#include "log.h"
#include "shared.h"

struct Fsuipc::Data {
	CRITICAL_SECTION critSect;
};


static Fsuipc *gFsuipc = 0;

Fsuipc::Fsuipc() {
	d = new Data;
	InitializeCriticalSection( &d->critSect );

	// connect to FSUIPC
	DWORD res;
	if( !FSUIPC_Open( SIM_ANY, &res ) )
		dlog << "Cannot connect to FSUIPC" << std::endl;
}


Fsuipc::~Fsuipc() {
	// disconnect from FSUIPC
	FSUIPC_Close();

	if( gFsuipc==this ) gFsuipc = 0;

	DeleteCriticalSection( &d->critSect );
	delete d;
}


SmartPtr<Fsuipc> Fsuipc::fsuipc() {
	gFsuipc = new Fsuipc();
	return gFsuipc;
}


void Fsuipc::begin() {
	EnterCriticalSection( &d->critSect );
}


bool Fsuipc::read( WORD id, unsigned size, void *data ) {
	DWORD res;
	return FSUIPC_Read( id, size, data, &res )==TRUE;
}


bool Fsuipc::write( WORD id, unsigned size, void *data ) {
	DWORD res;
	return FSUIPC_Write( id, size, data, &res )==TRUE;
}


bool Fsuipc::end() {	
	DWORD res;
	bool ok = FSUIPC_Process( &res )==TRUE;	
	LeaveCriticalSection( &d->critSect );
	return ok;
}
