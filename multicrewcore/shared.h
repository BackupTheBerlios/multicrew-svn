/*
 *  Copyright (C) 2002 Stefan Schimanski <1Stein@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef MULTICREWCORE_SHARED_H_INCLUDED
#define MULTICREWCORE_SHARED_H_INCLUDED

//#define SHARED_DEBUG

#pragma warning (disable : 4231)
#pragma warning (disable : 4251)

#include "common.h"
#include "streams.h"

#ifdef SHARED_DEBUG
#include <set>
#endif


class SharedBase {
 public:
  virtual int ref()=0;
  virtual int deref()=0;
};


class DLLEXPORT Shared : public SharedBase {
 public:
  Shared() { 
#ifdef SHARED_DEBUG
	  dout << (void*)this << ":" << typeid(*this).name() 
		   << " constructor" << std::endl;
#endif
	  refcount = 0; 
  }
  virtual ~Shared() {
#ifdef SHARED_DEBUG
	  dout << (void*)this << ":" << typeid(*this).name() 
		   << " destructor" << std::endl;
#endif
  }

  int ref();
  int deref();

 private:
  int refcount;
};


class DLLEXPORT SmartPtrBase {
 public:
	SmartPtrBase( char *file, int line ) {
#ifdef SHARED_DEBUG
		this->file = file;
		this->line = line;
		pointers.insert( this );
#endif
	}

	SmartPtrBase() {
#ifdef SHARED_DEBUG
		this->file = "";
		this->line = 0;
		pointers.insert( this );
#endif
	}

	virtual ~SmartPtrBase() {
#ifdef SHARED_DEBUG
		pointers.erase( this );
#endif
	}

#ifdef SHARED_DEBUG
	char *file;
	int line;

	static void dumpPointers() {
		std::set<SmartPtrBase*>::iterator it = pointers.begin();
		while( it!=pointers.end() ) {
			dout << "SmartPtr leak " << (*it)->file << ":" << (*it)->line << std::endl;
			it++;
		}
	}

 private:
	static std::set<SmartPtrBase*> pointers;

#endif
};

template< class T >
class SmartPtr : public SmartPtrBase {
 public:
  SmartPtr( char *file, int line ) : SmartPtrBase( file, line ) {
    obj = 0;
  }

  SmartPtr() : SmartPtrBase() {
    obj = 0;
  }

  SmartPtr( T *other ) : SmartPtrBase() {
    obj = (SharedBase*)other;
    if( obj ) obj->ref();
  }

  SmartPtr( const SmartPtr<T>& other ) : SmartPtrBase() {
    obj = other.obj;
    if( obj ) obj->ref();
  }

  virtual ~SmartPtr() {
    if( obj ) obj->deref();
  }

  SmartPtr<T> &operator=( const SmartPtr<T> &other ) {
    if( other.obj!=obj ) {
      if( obj ) obj->deref();
      obj = other.obj;
      if( obj ) obj->ref();
    }

    return *this;
  }

  SmartPtr<T> &operator=( T* other ) {
    if( other!=obj ) {
      if( obj ) obj->deref();
      obj = (SharedBase*)other;
      if( obj ) obj->ref();
    }

    return *this;
  }

  T* operator->() const { return (T*)obj; }
  //T* operator*() const { return (T*)obj; }
  operator T*() const { return (T*)obj; }
  T* operator&() const { return (T*)obj; }

  bool operator==( const SmartPtr<T> &other ) const {
    return obj==other.obj;
  }

  bool operator!=( const SmartPtr<T> &other ) const {
    return !( *this==other );
  }

  bool isNull() const {
    return obj==0;
  }

 private:
  SharedBase *obj;
};


class SharedAdaptor : public SharedBase {
 public:
  SharedAdaptor( SharedBase *p ) {
    parent = p;
  }

 private:
  SharedBase *parent;

  int ref() {
    if( parent ) 
      return parent->ref(); 
    else 
      return 0;
  }

  virtual int deref() {
    if( parent ) 
      return parent->deref(); 
    else 
      return 0;
  }
};


#endif
