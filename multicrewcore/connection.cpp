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

#include "network.h"

// {B1449AC0-D32F-4b58-81E7-AA5F5D424CBD}
const GUID gMulticrewGuid = 
{ 0xb1449ac0, 0xd32f, 0x4b58, { 0x81, 0xe7, 0xaa, 0x5f, 0x5d, 0x42, 0x4c, 0xbd } };


void Connection::addReceiver( Receiver *receiver ) {
	receivers[receiver->id()] = receiver;
}


void Connection::removeReceiver( Receiver *receiver ) {
	receivers.erase( receivers.find(receiver->id()) );
}


void Connection::deliverModulePacket( ModulePacket *packet ) {
	// find destination
	std::map<std::string,Receiver*>::iterator dest;
	dest = receivers.find(packet->module);
	if( dest==receivers.end() ) {
		dout << "Unroutable packet for module \"" << packet->module << "\"" << std::endl;
		return;
	}

	// deliver packet
	(*dest).second->receive(packet);
}
