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

#include <deque>


class DLLEXPORT Buffer : public Shared {
 public:
	Buffer( unsigned size ) {
		_copy = true;
		_data = malloc(size);
		_size = size;
	}

	Buffer( void *data, unsigned size, bool copy=true ) {
		_copy = copy;
		if( copy ) {
			this->_data = malloc( size );
			this->_size = size;
			memcpy( _data, data, size );
		} else {
			this->_data = data;
			this->_size = size;
		}
	}

	virtual ~Buffer() {
		if( _copy ) free(_data);
	}

	void *data() {
		return _data;
	}

	void *data( unsigned start ) {
		return ((char*)_data)+start;
	}

	unsigned size() {
		return _size;
	}

 private:
	bool _copy;
	void *_data;
	unsigned _size;
};


class DLLEXPORT SharedBuffer {
 private:
	friend Buffer;
	unsigned _start;
	unsigned _size;
	SmartPtr<Buffer> buf;

 public:	
	SharedBuffer( void *data, unsigned size, bool copy=true ) {
		buf = new Buffer( data, size, copy );
		_start = 0;
		this->_size = size;
	}

	SharedBuffer( unsigned size ) {
		buf = new Buffer( size );
		_start = 0;
		this->_size = size;
	}

	SharedBuffer( SmartPtr<Buffer> buf ) {
		this->buf = buf;
		this->_start = 0;
		this->_size = buf->size();
	}

	SharedBuffer( SmartPtr<Buffer> buf, unsigned start, unsigned size ) {
		this->buf = buf;
		this->_start = _start;
		this->_size = size;
	}

	SharedBuffer( SharedBuffer &buf ) {
		this->buf = buf.buf;
		this->_start = buf._start;
		this->_size = buf._size;
	}

	SharedBuffer( SharedBuffer &buf, unsigned start ) {
		this->buf = buf.buf;
		this->_start = buf._start+start;
		this->_size = buf._size-start;
	}

	SharedBuffer( SharedBuffer &buf, unsigned start, unsigned size ) {
		this->buf = buf.buf;
		this->_start = buf._start+start;
		this->_size = size;
	}

	void *data() {
		return buf->data( _start );
	}

	void *data( unsigned start ) {
		return buf->data( _start+start );
	}

	unsigned size() {
		return _size;
	}
};


class Packet : public Shared {
 public:
	virtual unsigned compiledSize()=0;
	virtual void compile( void *buffer )=0;
};


template< class Prefix, class Wrappee >
class DLLEXPORT WrappedPacket : public Packet {
 public:
	WrappedPacket( Prefix *prefix, SmartPtr<Wrappee> wrappee ) 
		: buffer(sizeof(Prefix)+wrappee->compiledSize()) {
		memcpy( buffer.data(), prefix, sizeof(Prefix) );
		wrappee->compile( buffer.data(sizeof(Prefix)) );
	}

	WrappedPacket( SharedBuffer &buf ) 
		: buffer( buf ) {
	}

	virtual ~WrappedPacket() {
	}

	Prefix *prefix() {
		return (Prefix*)buffer.data();
	}

	virtual unsigned compiledSize() {
		return buffer.size();
	}

	virtual void compile( void *buffer ) {
		memcpy( buffer, this->buffer.data(), this->buffer.size() );
	}

	SmartPtr<Wrappee> wrappee() {
		return createWrappee( SharedBuffer(buffer, sizeof(Prefix)) );
	}

	static Prefix *prefix( SharedBuffer &buffer ) {
		return (Prefix*)buffer.data();
	}

 protected:
	virtual SmartPtr<Wrappee> createWrappee( SharedBuffer &buffer )=0;

 private:
	SharedBuffer buffer;
};


template< class PacketBase >
class DLLEXPORT ArrayPacket : public Packet {
 public:
	typedef SmartPtr<PacketBase> SmartPacketBase;

	ArrayPacket() 
		: buffer( 1 ) {
		compiled = false;
	}

	ArrayPacket( SharedBuffer &buf ) 
		: buffer( buf ) {
		compiled = true;
	}

	void append( SmartPtr<PacketBase> packet ) {
		packets.push_back( packet );
	}

	virtual unsigned compiledSize() {
		unsigned ret = 0;
		std::deque<SmartPacketBase>::iterator it = packets.begin();
		while( it!=packets.end() ) {
			ret += (*it)->compiledSize();
			it++;
		}
		return ret  + sizeof(unsigned) + sizeof(unsigned)*packets.size();
	}

	virtual void compile( void *buffer ) {
		// write number of items
		char *buf = (char*)buffer;
		*((unsigned*)buf) = packets.size();
		unsigned pos = sizeof(unsigned);

		// write items
		std::deque<SmartPacketBase>::iterator it = packets.begin();
		while( it!=packets.end() ) {
			*(unsigned*)(buf + pos) = (*it)->compiledSize();
			pos += sizeof(unsigned);
			(*it)->compile( buf+pos );
			pos += (*it)->compiledSize();

			it++;
		}
	}

	typedef std::deque<SmartPacketBase>::iterator iterator;
	iterator begin() {
		if( compiled ) decompile();
		return packets.begin();
	}
	iterator end() {
		if( compiled ) decompile();
		return packets.end();
	}
	unsigned size() {
		if( compiled ) decompile();
		return packets.size();
	}

 protected:
	virtual SmartPtr<PacketBase> createChild( SharedBuffer &buffer )=0;
	
 private:
	bool compiled;
	SharedBuffer buffer;
	std::deque<SmartPacketBase> packets;

	void decompile() {
		// decompile the buffer
		unsigned size = *(unsigned*)buffer.data();
		unsigned pos = sizeof(unsigned);
		for( int i=0; i<size; i++ ) {
			unsigned packetSize = *(unsigned*)(buffer.data(pos));
			pos += sizeof(unsigned);

			SmartPacketBase packet = createChild( SharedBuffer(buffer,pos,packetSize) );
			pos += packetSize;
			if( !packet.isNull() )
				packets.push_back( packet );
		}

		compiled = false;
	}
};


template< class Key >
struct DLLEXPORT TypePrefix {
	TypePrefix( Key key ) {
		this->key = key;
	}

	TypePrefix() {}

	Key key;
};


template< class PacketBase >
class PacketFactory {
 public:
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer )=0;
};


template< class Key, class PacketBase >
class TypedPacketFactory : public PacketFactory<PacketBase> {
 public:
	virtual SmartPtr<PacketBase> createPacket( Key key, SharedBuffer &buffer )=0;
	
 private:
	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer ) {
		return createPacket( TypedPacket<Key,PacketBase>::prefix(buffer)->key,
							 SharedBuffer( buffer, sizeof(TypePrefix<Key>) ) );
	}
};


template< class Key, class PacketBase >
class DLLEXPORT PacketRegistryFactory : public TypedPacketFactory<Key, PacketBase>, public Shared {
 public:
	virtual SmartPtr<PacketBase> createPacket( Key key, SharedBuffer &buffer ) {
		std::map<Key, FactoryFunction>::iterator it = factories.find( key );
		if( it!=factories.end() ) return (it->second)( buffer );
		return 0;
	}

	typedef SmartPtr<PacketBase> (*FactoryFunction)( SharedBuffer &buffer );

	void registerKey( Key key, FactoryFunction factory ) {
		factories[key] = factory;
	}

 private:
	std::map<Key, FactoryFunction> factories;
};


template< class Key, class PacketBase >
class DLLEXPORT TypedPacket : public WrappedPacket<TypePrefix<Key>, PacketBase> {
 public:
	typedef TypedPacketFactory<Key, PacketBase> Factory;
	TypedPacket( SharedBuffer &buffer, Factory *factory )
		: WrappedPacket<TypePrefix<Key>, PacketBase>( buffer ) {
		this->factory = factory;
	}

	TypedPacket( Key key, SmartPtr<PacketBase> packet ) 
		: WrappedPacket<TypePrefix<Key>, PacketBase>( new TypePrefix<Key>(key), packet ) {
	}

	Key key() {
		return prefix()->key;
	}

	static key( SharedBuffer &buffer ) {
		return WrappedPacket<TypePrefix<Key>,PacketBase>::prefix(buffer)->key;
	}

 protected:
	virtual SmartPtr<PacketBase> createWrappee( SharedBuffer &buffer ) {
		return factory->createPacket( prefix()->key, buffer );
	}
	
 private:
	Factory *factory;
};


class RawPacket : public Packet {
public:
	RawPacket( void *data, unsigned size ) 
		: buffer( data, size, true ) {
	}

	virtual ~RawPacket() {
	}
	
	virtual unsigned compiledSize() {
		return buffer.size() + sizeof(unsigned);
	}

	virtual void compile( void *buffer ) {
		*((unsigned*)buffer) = this->buffer.size();
		memcpy( ((char*)buffer)+sizeof(unsigned), 
				this->buffer.data(), 
				this->buffer.size() );
	}

private:
	Buffer buffer;
};


template< class T >
class StructPacket : public Packet {
public:
	StructPacket( T &data ) {
		memcpy( &_data, &data, sizeof(T) );
	}	

	StructPacket( SharedBuffer &buf ) {
		memcpy( &_data, buf.data(), sizeof(T) );
	}

	virtual unsigned compiledSize() {
		return sizeof(T);
	}

	virtual void compile( void *buf ) {
		memcpy( buf, &_data, sizeof(T) );
	}

	T &data() {
		return _data;
	}

 private:
	T _data;
};

#endif
