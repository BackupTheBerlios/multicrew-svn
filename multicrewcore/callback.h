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

#ifndef CALLBACK_H_INCLUDED
#define CALLBACK_H_INCLUDED

#include "common.h"

class DLLEXPORT CallbackAdapterBase {
public:
	CallbackAdapterBase( void *methodThunk );
	virtual ~CallbackAdapterBase();
	void *callback();

protected:
	void setTarget( void *target );
	
private:
	struct Data;
	Data *d;
};

template <class R, class C>
class CallbackAdapter : public CallbackAdapterBase {
public:
	typedef R (__stdcall *Callback)();
	typedef R (C::*Member)();
	CallbackAdapter( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	CallbackAdapter( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static R __stdcall methodThunk() {
		CallbackAdapter<R, C> *_this;
		__asm mov _this, edx
		return ((_this->target)->*(_this->member))();
	}
};

template <class R, class C, class P1>
class CallbackAdapter1 : public CallbackAdapterBase {
public:
	typedef R (__stdcall *Callback)(P1);
	typedef R (C::*Member)( P1 );
	CallbackAdapter1( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	CallbackAdapter1( Member member) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static R __stdcall methodThunk( P1 p1 ) {
		CallbackAdapter1<R, C, P1> *_this;
		__asm mov _this, edx
		return ((_this->target)->*(_this->member))( p1 );
	}
};

template <class R, class C, class P1, class P2>
class CallbackAdapter2 : public CallbackAdapterBase {
public:
	typedef R (__stdcall *Callback)(P1, P2);
	typedef R (C::*Member)( P1, P2 );
	CallbackAdapter2( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	CallbackAdapter2( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static R __stdcall methodThunk( P1 p1, P2 p2 ) {
		CallbackAdapter2<R, C, P1, P2> *_this;
		__asm mov _this, edx
		return ((_this->target)->*(_this->member))( p1, p2 );
	}
};

template <class R, class C, class P1, class P2, class P3>
class CallbackAdapter3 : public CallbackAdapterBase {
public:
	typedef R (__stdcall *Callback)(P1, P2, P3);
	typedef R (C::*Member)( P1, P2, P3 );
	CallbackAdapter3( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}	
	CallbackAdapter3( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static R __stdcall methodThunk( P1 p1, P2 p2, P3 p3 ) {
		CallbackAdapter3<R, C, P1, P2, P3> *_this;
		__asm mov _this, edx
		return ((_this->target)->*(_this->member))( p1, p2, p3 );
	}
};

template <class R, class C, class P1, class P2, class P3, class P4>
class CallbackAdapter4 : public CallbackAdapterBase {
public:
	typedef R (__stdcall *Callback)(P1, P2, P3, P4);
	typedef R (C::*Member)( P1, P2, P3, P4 );
	CallbackAdapter4( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	CallbackAdapter4( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static R __stdcall methodThunk( P1 p1, P2 p2, P3 p3, P4 p4 ) {
		CallbackAdapter4<R, C, P1, P2, P3, P4> *_this;
		__asm mov _this, edx
		return ((_this->target)->*(_this->member))( p1, p2, p3, p4 );
	}
};


template <class C>
class VoidCallbackAdapter : public CallbackAdapterBase {
public:
	typedef void (__stdcall *Callback)();
	typedef void (C::*Member)();
	VoidCallbackAdapter( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) { 
		this->target = target;
		this->member=member; 
	}
	VoidCallbackAdapter( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

 private:
	C *target;
	Member member;
	static void __stdcall methodThunk() {
		VoidCallbackAdapter<C> *_this;
		__asm mov _this, edx
		((_this->target)->*(_this->member))();
	}
};

template <class C, class P1>
class VoidCallbackAdapter1 : public CallbackAdapterBase {
public:
	typedef void (__stdcall *Callback)(P1);
	typedef void (C::*Member)( P1 );
	VoidCallbackAdapter1( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	VoidCallbackAdapter1( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static void __stdcall methodThunk( P1 p1 ) {
		VoidCallbackAdapter1<C, P1> *_this;
		__asm mov _this, edx
		((_this->target)->*(_this->member))( p1 );
	}
};

template <class C, class P1, class P2>
class VoidCallbackAdapter2 : public CallbackAdapterBase {
public:
	typedef void (__stdcall *Callback)(P1, P2);
	typedef void (C::*Member)( P1, P2 );
	VoidCallbackAdapter2( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	VoidCallbackAdapter2( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static void __stdcall methodThunk( P1 p1, P2 p2 ) {
		VoidCallbackAdapter2<C, P1, P2> *_this;
		__asm mov _this, edx
		((_this->target)->*(_this->member))( p1, p2 );
	}
};

template <class C, class P1, class P2, class P3>
class VoidCallbackAdapter3 : public CallbackAdapterBase {
public:
	typedef void (__stdcall *Callback)(P1, P2, P3);
	typedef void (C::*Member)( P1, P2, P3 );
	VoidCallbackAdapter3( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}	
	VoidCallbackAdapter3( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static void __stdcall methodThunk( P1 p1, P2 p2, P3 p3 ) {
		VoidCallbackAdapter3<C, P1, P2, P3> *_this;
		__asm mov _this, edx
		((_this->target)->*(_this->member))( p1, p2, p3 );
	}
};

template <class C, class P1, class P2, class P3, class P4>
class VoidCallbackAdapter4 : public CallbackAdapterBase {
public:
	typedef void(__stdcall *Callback)(P1, P2, P3, P4);
	typedef void (C::*Member)( P1, P2, P3, P4 );
	VoidCallbackAdapter4( C *target, Member member ) 
		: CallbackAdapterBase( &methodThunk ) {
		this->target = target;
		this->member = member;
	}
	VoidCallbackAdapter4( Member member ) : CallbackAdapterBase( NULL, &methodThunk ) {
		this->target = 0;
		this->member = member;
	}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	C *target;
	Member member;
	static void __stdcall methodThunk( P1 p1, P2 p2, P3 p3, P4 p4 ) {
		VoidCallbackAdapter4<C, P1, P2, P3, P4> *_this;
		__asm mov _this, edx
		((_this->target)->*(_this->member))( p1, p2, p3, p4 );
	}
};

/*
template <class R, class C, R (C::*member)()>
class ThisCallbackAdapter : public CallbackAdapterBase {
public:
	typedef R (__thiscall *Callback)();
	ThisCallbackAdapter( C *target ) : CallbackAdapterBase( target, &methodThunk ) {}
	ThisCallbackAdapter() : CallbackAdapterBase( NULL, &methodThunk ) {}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	static R __thiscall methodThunk() {
		C *c = (C*)callee;	
		return (c->*member)();
	}
};

template <class R, class C, class P1, R (C::*member)(P1)>
class ThisCallbackAdapter1 : public CallbackAdapterBase {
public:
	typedef R (__thiscall *Callback)(P1);
	ThisCallbackAdapter1( C *target ) : CallbackAdapterBase( target, &methodThunk ) {}
	ThisCallbackAdapter1() : CallbackAdapterBase( NULL, &methodThunk ) {}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	static R __thiscall methodThunk( P1 p1 ) {
		C *c = (C*)callee;
		return (c->*member)( p1 );
	}
};

template <class R, class C, class P1, class P2, R (C::*member)(P1, P2)>
class ThisCallbackAdapter2 : public CallbackAdapterBase {
public:
	typedef R (__thiscall *Callback)(P1, P2);
	ThisCallbackAdapter2( C *target ) : CallbackAdapterBase( target, &methodThunk ) {}
	ThisCallbackAdapter2() : CallbackAdapterBase( NULL, &methodThunk ) {}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	static R __thiscall methodThunk( P1 p1, P2 p2 ) {
		C *c = (C*)callee;
		return (c->*member)( p1, p2 );
	}
};

template <class R, class C, class P1, class P2, class P3, R (C::*member)(P1, P2, P3)>
class ThisCallbackAdapter3 : public CallbackAdapterBase {
public:
	typedef R (__thiscall *Callback)(P1, P2, P3);
	ThisCallbackAdapter3( C *target ) : CallbackAdapterBase( target, &methodThunk ) {}	
	ThisCallbackAdapter3() : CallbackAdapterBase( NULL, &methodThunk ) {}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	static R __thiscall methodThunk( P1 p1, P2 p2, P3 p3 ) {
		C *c = (C*)callee;
		return (c->*member)( p1, p2, p3 );
	}
};

template <class R, class C, class P1, class P2, class P3, class P4, R (C::*member)(P1, P2, P3, P4)>
class ThisCallbackAdapter4 : public CallbackAdapterBase {
public:
	typedef R (__thiscall *Callback)(P1, P2, P3, P4);
	ThisCallbackAdapter4( C *target ) : CallbackAdapterBase( target, &methodThunk ) {}
	ThisCallbackAdapter4() : CallbackAdapterBase( NULL, &methodThunk ) {}
	Callback callback() { return (Callback)CallbackAdapterBase::callback(); }
	void setTarget( C *target ) { CallbackAdapterBase::setTarget( target ); }

private:
	static R __thiscall methodThunk( P1 p1, P2 p2, P3 p3, P4 p4 ) {
		C *c = (C*)callee;
		return (c->*member)( p1, p2, p3, p4 );
	}
};
*/

#endif
