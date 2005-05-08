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

#ifndef MULTICREW_METAFILE_H_INCLUDED
#define MULTICREW_METAFILE_H_INCLUDED


#include "../multicrewcore/shared.h"
#include "../multicrewcore/packets.h"
#include "../multicrewcore/thread.h"

#include "zdelta/zdlib.h"


class GlobalMem : public Shared {
public:
	GlobalMem( HGLOBAL hmem ) {
		this->hmem = hmem;
	}

	GlobalMem( unsigned size ) {
		this->hmem = GlobalAlloc( 0, size );
	}

	GlobalMem( void *data, unsigned size ) {
		this->hmem = GlobalAlloc( 0, size );
		memcpy( lock(), data, size );
		unlock();
	}

	virtual ~GlobalMem() {
		GlobalFree( hmem );
	}

	void *lock() {
		return GlobalLock( hmem );
	}

	void unlock() {
		GlobalUnlock( hmem );
	}

	unsigned size() {
		return GlobalSize( hmem );
	}

	HGLOBAL handle() {
		return hmem;
	}

private:
	HGLOBAL hmem;
};


class GaugeModule;
class MetafileCompressor : public Shared, private Thread {
 public:
	MetafileCompressor( GaugeModule *mgauge, int delay, int channel );
	virtual ~MetafileCompressor();

	virtual void receive( SmartPtr<PacketBase> packet );	
	bool open();
	void close( SmartPtr<GlobalMem> metafile );

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

 protected:
	int zd_compress1(const Bytef *ref, uLong rsize,
					 const Bytef *tar, uLong tsize,
					 Bytef **delta, uLongf *dsize);	

	virtual unsigned threadProc( void *param );
	virtual void boostMetafileThread( HWND, UINT, UINT_PTR, DWORD);

 private:
	struct Data;
	friend Data;
	Data *d;
};


class MetafileDecompressor : public Shared {
 public:
	MetafileDecompressor( GaugeModule *mgauge, int channel );
	virtual ~MetafileDecompressor();

	virtual void receive( SmartPtr<PacketBase> packet );
	SmartPtr<GlobalMem> lastMetafile();
	int counter();

	virtual SmartPtr<PacketBase> createPacket( SharedBuffer &buffer );

 private:
	struct Data;
	friend Data;
	Data *d;
};

#endif
