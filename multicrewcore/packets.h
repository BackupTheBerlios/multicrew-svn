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

#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include "../multicrewgauge/gauges.h"

#pragma pack(push,1)

enum PacketIds {
	startPacket = 0,
	startAckPacket,
	firstModulePacket,
	positionPacket,
	firstUpdatePacket,
	iconUpdatePacket,
	needleUpdatePacket,
	stringUpdatePacket,
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
	ModulePacket( std::string module, int id, unsigned size ) : Packet( id, size ) {
		strcpy( this->module, module.c_str() );
	}

	char module[32];
};

struct PositionPacket : public ModulePacket {
	PositionPacket( std::string module ) 
		: ModulePacket(module,positionPacket,sizeof(PositionPacket)) {
	}
	
	DWORD lat[2];
	DWORD lon[2];
	DWORD alt[2];
	DWORD dir[3];
	FLOAT64 accel[6];
	FLOAT64 vel[6];
};


// update packets
struct UpdatePacket : public ModulePacket {
	UpdatePacket( std::string module, int gauge, int element, int id, unsigned size ) 
		: ModulePacket(module,id,size) {
		this->gauge=gauge;
		this->element=element;
	}
	int gauge;
	int element;
};

struct IconUpdatePacket : public UpdatePacket {
	IconUpdatePacket( std::string module, int gauge, int element, FLOAT64 value ) 
		: UpdatePacket(module,gauge,element,iconUpdatePacket,sizeof(IconUpdatePacket)) {
			this->value=value;
	}
	FLOAT64 value;
};

struct NeedleUpdatePacket : public UpdatePacket {
	NeedleUpdatePacket( std::string module, int gauge, int element, FLOAT64 value ) 
		: UpdatePacket(module,gauge,element,needleUpdatePacket,sizeof(NeedleUpdatePacket)) {
			this->value=value;
	}
	FLOAT64 value;
};

struct StringUpdatePacket : public UpdatePacket {
	StringUpdatePacket( std::string module, int gauge, int element, char *value ) 
		: UpdatePacket(module,gauge,element,stringUpdatePacket,sizeof(StringUpdatePacket)) {
		strcpy( this->value, value );
	}
	char value[32];
};



#pragma pack(pop,1)

#endif
