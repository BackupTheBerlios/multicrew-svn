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

#ifndef MULTICREWCORE_FSUIPCMODULE_H_INCLUDED
#define MULTICREWCORE_FSUIPCMODULE_H_INCLUDED

#include "multicrewcore.h"


class FsuipcModule : public MulticrewModule {
 public:
	FsuipcModule( bool hostMode );
	virtual ~FsuipcModule();

	virtual void handlePacket( SmartPtr<PacketBase> packet );
	virtual void sendProc();
	virtual void sendFullState();

	void watch( WORD id, BYTE size, bool safe, bool highPrioo=false );
	
 protected:
	virtual SmartPtr<PacketBase> createInnerModulePacket( SharedBuffer &buffer );	

 private:
	struct Data;
	friend Data;
	Data *d;	
};


#endif
