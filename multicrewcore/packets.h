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
#include <map>


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

	SharedBuffer( const SharedBuffer &buf ) {
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


class PacketBase : public Shared {
 public:
	virtual unsigned compiledSize() { return 0; }
	virtual void compile( void *buffer ) {};
};


class EmptyPacket : public PacketBase {};


template< class Prefix, class Wrappee >
class DLLEXPORT WrappedPacket : public PacketBase {
 public:
	WrappedPacket( const Prefix &prefix, SmartPtr<Wrappee> wrappee ) 
		: buffer(sizeof(Prefix)+wrappee->compiledSize()) {
		memcpy( buffer.data(), &prefix, sizeof(Prefix) );
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

	virtual SmartPtr<Wrappee> wrappee() {
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


template< class P >
class DLLEXPORT ArrayPacket : public PacketBase {
 public:
	typedef SmartPtr<P> SmartP;

	ArrayPacket() 
		: buffer( 1 ) {
		compiled = false;
	}

	ArrayPacket( SharedBuffer &buf ) 
		: buffer( buf ) {
		compiled = true;
	}

	void append( SmartPtr<P> packet ) {
		packets.push_back( packet );
	}

	virtual unsigned compiledSize() {
		unsigned ret = 0;
		std::deque<SmartP>::iterator it = packets.begin();
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
		std::deque<SmartP>::iterator it = packets.begin();
		while( it!=packets.end() ) {
			*(unsigned*)(buf + pos) = (*it)->compiledSize();
			pos += sizeof(unsigned);
			(*it)->compile( buf+pos );
			pos += (*it)->compiledSize();

			it++;
		}
	}

	typedef std::deque<SmartP>::iterator iterator;
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
	virtual SmartPtr<P> createChild( SharedBuffer &buffer )=0;
	
 private:
	bool compiled;
	SharedBuffer buffer;
	std::deque<SmartP> packets;

	void decompile() {
		// decompile the buffer
		unsigned size = *(unsigned*)buffer.data();
		unsigned pos = sizeof(unsigned);
		for( int i=0; i<size; i++ ) {
			unsigned packetSize = *(unsigned*)(buffer.data(pos));
			pos += sizeof(unsigned);

			SmartP packet = createChild( SharedBuffer(buffer,pos,packetSize) );
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


template< class P >
class PacketFactory {
 public:
	virtual SmartPtr<P> createPacket( SharedBuffer &buffer )=0;
};


template< class Key, class P >
class TypedPacketFactory : public PacketFactory<P> {
 public:
	virtual SmartPtr<P> createPacket( Key key, SharedBuffer &buffer )=0;

 private:
	virtual SmartPtr<P> createPacket( SharedBuffer &buffer ) {
		return createPacket(
			TypedPacket<Key,P>::prefix(buffer)->key,
			SharedBuffer( buffer, sizeof(TypePrefix<Key>) ) );
	}
};


template< class Key, class P >
class DLLEXPORT PacketRegistryFactory : public TypedPacketFactory<Key, P>, public Shared {
 public:
	virtual SmartPtr<P> createPacket( Key key, SharedBuffer &buffer ) {
		std::map<Key, FactoryFunction>::iterator it = factories.find( key );
		if( it!=factories.end() ) return (it->second)( buffer );
		return 0;
	}

	typedef SmartPtr<P> (*FactoryFunction)( SharedBuffer &buffer );

	void registerKey( Key key, FactoryFunction factory ) {
		factories[key] = factory;
	}

 private:
	std::map<Key, FactoryFunction> factories;
};


template< class Key, class P >
class DLLEXPORT TypedPacket : public WrappedPacket<TypePrefix<Key>, P> {
 public:
	typedef TypedPacketFactory<Key, P> Factory;
	TypedPacket( SharedBuffer &buffer, Factory *factory )
		: WrappedPacket<TypePrefix<Key>, P>( buffer ) {
		this->factory = factory;
	}

	TypedPacket( Key key, SmartPtr<P> packet ) 
		: WrappedPacket<TypePrefix<Key>, P>( TypePrefix<Key>(key), packet ) {
	}

	Key key() {
		return prefix()->key;
	}

	static key( SharedBuffer &buffer ) {
		return WrappedPacket<TypePrefix<Key>,P>::prefix(buffer)->key;
	}

 protected:
	virtual SmartPtr<P> createWrappee( SharedBuffer &buffer ) {
		return factory->createPacket( prefix()->key, buffer );
	}
	
 private:
	Factory *factory;
};


class RawPacket : public PacketBase {
public:
	RawPacket( void *data, unsigned size ) 
		: _buffer( data, size, true ) {
	}

	RawPacket( const SharedBuffer &buffer )
		: _buffer( buffer ) {
	}

	virtual ~RawPacket() {
	}
	
	virtual unsigned compiledSize() {
		return _buffer.size();
	}

	virtual void compile( void *buffer ) {
		memcpy( ((char*)buffer), this->_buffer.data(), this->_buffer.size() );
	}

	SharedBuffer buffer() {
		return _buffer;
	}

private:
	SharedBuffer _buffer;
};


template< class T >
class StructPacket : public PacketBase {
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


template< class P >
class StringTypedPacketFactory : public PacketFactory<P> {
 public:
	virtual SmartPtr<P> createPacket( std::string key, SharedBuffer &buffer )=0;
	
 private:
	virtual SmartPtr<P> createPacket( SharedBuffer &buffer ) {
		std::string key = StringTypedPacket<P>::key(buffer);
		return createPacket( key, SharedBuffer( buffer, key.length() ) );
	}
};


template< class Wrappee >
class DLLEXPORT StringWrappedPacket : public PacketBase {
 public:
	StringWrappedPacket( std::string key, SmartPtr<Wrappee> wrappee ) 
		: buffer(key.length()+1+wrappee->compiledSize()) {
		strcpy( (char*)buffer.data(), key.c_str() );
		wrappee->compile( buffer.data(key.length()+1) );
	}

	StringWrappedPacket( SharedBuffer &buf ) 
		: buffer( buf ) {
	}

	virtual ~StringWrappedPacket() {
	}

	std::string key() {
		return (char*)buffer.data();
	}

	virtual unsigned compiledSize() {
		return buffer.size();
	}

	virtual void compile( void *buffer ) {
		memcpy( buffer, this->buffer.data(), this->buffer.size() );
	}

	virtual SmartPtr<Wrappee> wrappee() {
		return createWrappee( SharedBuffer(buffer, strlen((char*)buffer.data())+1) );
	}

	static std::string key( SharedBuffer &buffer ) {
		return (char*)buffer.data();
	}

 protected:
	virtual SmartPtr<Wrappee> createWrappee( SharedBuffer &buffer )=0;

 private:
	SharedBuffer buffer;
};


template< class P >
class DLLEXPORT StringTypedPacket : public StringWrappedPacket<P> {
 public:
	typedef StringTypedPacketFactory<P> Factory;
	StringTypedPacket( SharedBuffer &buffer, Factory *factory )
		: StringWrappedPacket<P>( buffer ) {
		this->factory = factory;
	}

	StringTypedPacket( std::string key, SmartPtr<P> packet ) 
		: StringWrappedPacket<P>( key, packet ) {
	}

 protected:
	virtual SmartPtr<P> createWrappee( SharedBuffer &buffer ) {
		return factory->createPacket( key(), buffer );
	}
	
 private:
	Factory *factory;
};


#endif
