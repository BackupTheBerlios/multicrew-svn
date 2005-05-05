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

#include <windows.h>
#include <dplay8.h>

#include "streams.h"
#include "log.h"
#include "networkimpl.h"
#include "callback.h"


class RoutedModulePacket : public PacketBase {
public:
	RoutedModulePacket( SmartPtr<ModulePacket> packet, SmartPtr<MulticrewModule> mod ) {
		this->packet = packet;
		this->mod = mod;
		this->modName = mod->moduleName();
	}	

	RoutedModulePacket( SharedBuffer &buffer, SmartPtr<MulticrewModule> mod ) {
		this->mod = mod;
		this->modName = mod->moduleName();
		packet = new ModulePacket( SharedBuffer(buffer,modName.size()+1), mod );
	}

	virtual unsigned compiledSize() {
		return modName.length()+1+packet->compiledSize();
	}

	virtual void compile( void *data ) {
		strcpy( (char*)data, modName.c_str() );
		packet->compile( ((char*)data)+modName.length()+1 );
	}

	SmartPtr<MulticrewModule> module() {
		return mod;
	}

	SmartPtr<ModulePacket> modulePacket() {
		return packet;
	}

	static std::string moduleName( SharedBuffer &buffer ) {
		return (char*)buffer.data();
	}

private:	
	std::string modName;
	SmartPtr<ModulePacket> packet;
	SmartPtr<MulticrewModule> mod;
};

SmartPtr<PacketBase> ModulePacket::createChild( SharedBuffer &buffer ) {
	return mod->createPacket( buffer );
}


/*******************************************************************/


struct ConnectionImpl::Data {
	Data( ConnectionImpl *con ) {}

	bool disconnected;
	CRITICAL_SECTION critSect;
};


ConnectionImpl::ConnectionImpl() {
	d = new Data( this );
	d->disconnected = false;
	InitializeCriticalSection( &d->critSect );
}


ConnectionImpl::~ConnectionImpl() {
	stopThread();
	DeleteCriticalSection( &d->critSect );
	delete d;
}


void ConnectionImpl::addModule( MulticrewModule *module ) {
	EnterCriticalSection( &d->critSect );
	modules[module->moduleName()] = module;
	LeaveCriticalSection( &d->critSect );
}


void ConnectionImpl::removeModule( MulticrewModule *module ) {
	EnterCriticalSection( &d->critSect );
	modules.erase( modules.find(module->moduleName()) );
	LeaveCriticalSection( &d->critSect );
}


unsigned ConnectionImpl::threadProc( void *param ) {
	while( !shouldExit(10) ) {
		processImpl();
	}
	return 0;
}


void ConnectionImpl::processPacket( void *data, unsigned length ) {
	SharedBuffer packetBuf( ((char*)data)+1, length-1 );

	// find destination
	EnterCriticalSection( &d->critSect );
	std::map<std::string,MulticrewModule*>::iterator dest;
	std::string moduleId = RoutedModulePacket::moduleName( packetBuf );
	dest = modules.find( moduleId );
	if( dest==modules.end() ) {
		LeaveCriticalSection( &d->critSect );
		dout << "Unroutable packet for module \"" << moduleId << "\"" << std::endl;
		return;
	}			
	LeaveCriticalSection( &d->critSect );
	
	// create packet
	SmartPtr<RoutedModulePacket> routed = 
		new RoutedModulePacket( packetBuf, (MulticrewModule*)(dest->second) );
				
	// deliver packet
	if( !routed.isNull() )
		routed->module()->receive( routed->modulePacket() );
}


void ConnectionImpl::disconnect() {
	if( !d->disconnected ) {
		ref();
		d->disconnected = true;
		dlog << "Terminating session" << std::endl;
		disconnectImpl();
		disconnected.emit();
		deref();
	}
}


bool ConnectionImpl::start() {
	startThread( 0 );
	return true;
}


bool ConnectionImpl::send( SmartPtr<ModulePacket> packet, bool safe, 
						   Priority prio, SmartPtr<MulticrewModule> sender,
						   bool async, int channel ) {
	// connected?
	if( d->disconnected ) {
		if( !sender.isNull() ) sender->sendFailed();
		return false;
	}

	// prepare packet
	SmartPtr<RoutedModulePacket> routed = 
		new RoutedModulePacket( packet, sender );
	unsigned size = routed->compiledSize()+1;
	char *buffer = (char*)malloc( size );
	routed->compile( buffer+1 );
	buffer[0]=127;

	// set priority
	PacketPriority pprio = MEDIUM_PRIORITY;
	switch( prio ) {
	case lowPriority: pprio=LOW_PRIORITY; break;
	case mediumPriority: pprio=MEDIUM_PRIORITY; break;
	case highPriority: pprio=HIGH_PRIORITY; break;
	default: break;
	}

	// set reliability mode
	PacketReliability reliability;
	if( async ) {
		if( safe )
			reliability = RELIABLE_ORDERED;
		else
			reliability = UNRELIABLE_SEQUENCED;
	} else {
		if( safe )
			reliability = RELIABLE_ORDERED;
		else
			reliability = UNRELIABLE_SEQUENCED;
	}
				
	// send
	bool ok = sendImpl( (char*)buffer, 
						size, 
						pprio, 
						reliability, 
						channel );
	
	// free packet buffer
	free( buffer );

    // error?
	if( !ok ) {
		if( !sender.isNull() ) sender->sendFailed();
		dout << "Failed to send packet." << std::endl;
		return false;
	}

    if( !sender.isNull() && !async ) sender->sendCompleted();
	return true;
}
