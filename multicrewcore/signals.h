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

#ifndef SIGNALS_H_INCLUDED
#define SIGNALS_H_INCLUDED

#pragma warning (disable : 4231)
#pragma warning (disable : 4251)

#include "common.h"

#include <list>
#include <set>

#include "callback.h"
#include "debug.h"

class DLLEXPORT SlotBase;
class DLLEXPORT Signal;
template <class Q1> class DLLEXPORT Signal1;
template <class Q1, class Q2> class DLLEXPORT Signal2;
template <class Q1, class Q2, class Q3> class DLLEXPORT Signal3;
template <class Q1, class Q2, class Q3, class Q4> class DLLEXPORT Signal4;

DLLEXPORT_TEMPLATE template class DLLEXPORT std::list<void*>;
//DLLEXPORT_TEMPLATE template class DLLEXPORT std::allocator<void*>;
DLLEXPORT_TEMPLATE template class DLLEXPORT std::list<SlotBase*>;

class DLLEXPORT SlotBase {
 public:
	virtual void disconnect()=0;
 private:
//	virtual void callback()=0;
};

template<class P1>
class DLLEXPORT SlotBase1 {
 public:
	virtual void disconnect()=0;
 private:
	virtual void callback( P1 p1 )=0;
};

template <class C> 
class Slot : public SlotBase {
public:
	typedef void (C::*Member)();
	Slot( Signal *signal, C *target, void (C::*slot)() )
		: adapter( target, slot ) {		
		this->signal = signal;
		this->slot = slot;
		if( signal ) signal->connect( adapter.callback(), this );
	}

	~Slot() {
		signal->disconnect( adapter.callback(), this );
	}

	void connect( Signal *signal ) {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		this->signal = signal;
		if( this->signal ) signal->connect( adapter.callback(), this );
	}

	virtual void disconnect() {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		signal = 0;
	}

private:
	//virtual void callback() { (c->*slot)(); }
	Signal *signal;
	void (C::*slot)();
	VoidCallbackAdapter<C> adapter;
};


template <class C, class P1> 
class Slot1 : public SlotBase {
public:
	typedef void (C::*Member)(P1);
	Slot1( Signal1<P1> *signal, C *target, Member slot )
		: adapter( target, slot ) {
		this->signal = signal;
		if( signal ) signal->connect( adapter.callback(), this );
	}

	~Slot1() {
		signal->disconnect( adapter.callback(), this );
	}

	void connect( Signal1<P1> *signal ) {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		this->signal = signal;
		if( this->signal ) signal->connect( adapter.callback(), this );
	}

	virtual void disconnect() {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		signal = 0;
	}

private:
	Signal1<P1> *signal;
	VoidCallbackAdapter1<C, P1> adapter;
};


template <class C, class P1, class P2, void (C::*slot)(P1, P2)> 
class Slot2 : public SlotBase {
public:
	typedef void (C::*Member)(P1, P2);
	Slot2( Signal2<P1, P2> *signal, C *target, Member slot  )
		: adapter( target, slot ) {
		this->signal = signal;
		if( signal ) signal->connect( adapter.callback(), this );
	}

	~Slot2() {
		signal->disconnect( adapter.callback(), this );
	}

	void connect( Signal2<P1,P2> *signal ) {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		this->signal = signal;
		if( this->signal ) signal->connect( adapter.callback(), this );
	}

	virtual void disconnect() {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		signal = 0;
	}

private:
	Signal2<P1, P2> *signal;
	VoidCallbackAdapter2<C, P1, P2> adapter;
};


template <class C, class P1, class P2, class P3, void (C::*slot)(P1, P2, P3)> 
class Slot3 : public SlotBase {
public:
	typedef void (C::*Member)(P1,P2,P3);
	Slot3( Signal3<P1, P2, P3> *signal, C *target, Member slot )
		: adapter( target ) {
		this->signal = signal;
		if( signal ) signal->connect( adapter.callback(), this );
	}

	~Slot3() {
		signal->disconnect( adapter.callback(), this );
	}

	void connect( Signal3<P1,P2,P3> *signal ) {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		this->signal = signal;
		if( this->signal ) signal->connect( adapter.callback(), this );
	}

	virtual void disconnect() {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		signal = 0;
	}

private:
	Signal3<P1, P2, P3> *signal;
	VoidCallbackAdapter3<C, P1, P2, P3> adapter;
};


template < class C, class P1, class P2, class P3, class P4, void (C::*slot)(P1, P2, P3, P4)> 
class Slot4 : public SlotBase {
public:
	typedef void (C::*Member)(P1,P2,P3);
	Slot4( Signal4<P1, P2, P3, P4> *signal, C *target, Member slot )
		: adapter( target, slot ) {
		this->signal = signal;
		if( signal ) signal->connect( adapter.callback(), this );
	}

	~Slot4() {
		signal->disconnect( adapter.callback(), this );
	}

	void connect( Signal4<P1,P2,P3,P4> *signal ) {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		this->signal = signal;
		if( this->signal ) signal->connect( adapter.callback(), this );
	}

	virtual void disconnect() {
		if( this->signal ) signal->disconnect( adapter.callback(), this );
		signal = 0;
	}

private:
	Signal4<P1, P2, P3, P4> *signal;
	VoidCallbackAdapter4<C, P1, P2, P3, P4> adapter;
};


class DLLEXPORT Signal {
private:
	std::list<void*> callbacks;
	std::list<SlotBase*> slots;

public:
	~Signal() {
		std::list<SlotBase*>::iterator it = slots.begin();
		while( it!=slots.end() ) (*(it++))->disconnect();
	}

	void emit() {
		std::list<void*>::iterator it = callbacks.begin();
		while( it!=callbacks.end() ) ((Callback)*(it++))();
	}

	typedef void (__stdcall *Callback)();	
	
	void connect( Callback callback, SlotBase *slot ) {
		callbacks.push_back( callback );
		slots.push_back( slot );
	}

	void disconnect( Callback callback, SlotBase *slot ) {
		callbacks.remove( callback );
		slots.remove( slot );
	}
};


template <class Q1>
class DLLEXPORT Signal1 {
private:
	std::list<void*> callbacks;
	std::list<SlotBase*> slots;

public:
	~Signal1() {
		std::list<SlotBase*>::iterator it = slots.begin();
		while( it!=slots.end() ) (*(it++))->disconnect();
	}

	void emit( Q1 q1 ) {
		std::list<void*>::iterator it = callbacks.begin();
		while( it!=callbacks.end() ) ((Callback)*(it++))( q1 );	
	}

	typedef void (__stdcall *Callback)( Q1 );	
		
	void connect( Callback callback, SlotBase *slot ) {
		callbacks.push_back( callback );
		slots.push_back( slot );
	}

	void disconnect( Callback callback, SlotBase *slot ) {
		callbacks.remove( callback );
		slots.remove( slot );
	}
};


template <class Q1, class Q2>
class DLLEXPORT Signal2 {
private:
	std::list<void*> callbacks;
	std::list<SlotBase*> slots;

public:
	~Signal2() {
		std::list<SlotBase*>::iterator it = slots.begin();
		while( it!=slots.end() ) (*(it++))->disconnect();
	}

	void emit( Q1 q1, Q2 q2 ) {
		std::list<void*>::iterator it = callbacks.begin();
		while( it!=callbacks.end() ) ((Callback)*(it++))( q1, q2 );		
	}

	typedef void (__stdcall *Callback)( Q1, Q2 );	
		
	void connect( Callback callback, SlotBase *slot ) {
		callbacks.push_back( callback );
		slots.push_back( slot );
	}

	void disconnect( Callback callback, SlotBase *slot ) {
		callbacks.remove( callback );
		slots.remove( slot );
	}
};


template <class Q1, class Q2, class Q3>
class DLLEXPORT Signal3 {
private:
	std::list<void*> callbacks;
	std::list<SlotBase*> slots;
	
public:
	~Signal3() {
		std::list<SlotBase*>::iterator it = slots.begin();
		while( it!=slots.end() ) (*(it++))->disconnect();
	}

	void emit( Q1 q1, Q2 q2, Q3 q3 ) {
		std::list<void*>::iterator it = callbacks.begin();
		while( it!=callbacks.end() ) ((Callback)*(it++))( q1, q2, q3 );		
	}

	typedef void (__stdcall *Callback)( Q1, Q2, Q3 );	
		
	void connect( Callback callback, SlotBase *slot ) {
		callbacks.push_back( callback );
		slots.push_back( slot );
	}

	void disconnect( Callback callback, SlotBase *slot ) {
		callbacks.remove( callback );
		slots.remove( slot );
	}
};


template <class Q1, class Q2, class Q3, class Q4>
class DLLEXPORT Signal4 {
private:
	std::list<void*> callbacks;
	std::list<SlotBase*> slots;

public:
	~Signal4() {
		std::list<SlotBase*>::iterator it = slots.begin();
		while( it!=slots.end() ) (*(it++))->disconnect();
	}

	void emit( Q1 q1, Q2 q2, Q3 q3, Q4 q4 ) {
		std::list<void*>::iterator it = callbacks.begin();
		while( it!=callbacks.end() ) ((Callback)*(it++))( q1, q2, q3, q4 );		
	}

	typedef void (__stdcall *Callback)( Q1, Q2, Q3, Q4 );	
		
	void connect( Callback callback, SlotBase *slot ) {
		callbacks.push_back( callback );
		slots.push_back( slot );
	}

	void disconnect( Callback callback, SlotBase *slot ) {
		callbacks.remove( callback );
		slots.remove( slot );
	}
};


#endif SIGNALS_H_INCLUDED
