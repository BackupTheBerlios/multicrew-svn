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

#ifndef MULTICREWCORE_PACKETS_H_INCLUDED
#define MULTICREWCORE_PACKETS_H_INCLUDED

#pragma pack(push,1)

enum PacketIds {
	startPacket = 0,
	startAckPacket,
	modulePacket,
};

struct Packet {
	Packet( int id, unsigned size ) { 
		this->id=id; 
		this->size=size;
	}

	int id;
	unsigned size;
};

// start packets
struct StartPacket : public Packet {
	StartPacket( long planeChecksum )
		: Packet(startPacket,sizeof(StartPacket)) {
		this->planeChecksum=planeChecksum; 
	}

	long planeChecksum;
};

struct StartAckPacket : public Packet {
	StartAckPacket( long planeChecksum )
		: Packet(startAckPacket,sizeof(StartAckPacket)) {
		this->planeChecksum=planeChecksum; 
	}

	long planeChecksum;
};

// module packets
struct ModulePacket : public Packet {
	ModulePacket() : Packet( modulePacket, sizeof(ModulePacket) ) {
		strcpy( module, "" );
	}

	char module[32];
};

#pragma pack(pop,1)

#endif
