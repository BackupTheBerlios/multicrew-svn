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

#include "common.h"

#include <windows.h>

#include "multicrewgauge.h"
#include "metafile.h"
#include "../stlplus/source/string_utilities.hpp"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/log.h"

extern "C" {
#include "zdelta/zdlib.h"
#include "zdelta/zd_mem.h"
}

#define METAFILEDELAY 300


struct MetafileDataStruct {
	MetafileDataStruct( unsigned counter ) {
		this->counter = counter;
	}

	unsigned counter;
};


class MetafileDataPacket : public WrappedPacket<MetafileDataStruct, RawPacket> {
public:
	MetafileDataPacket( unsigned counter, SharedBuffer &buffer )
		: WrappedPacket<MetafileDataStruct, RawPacket>(
			MetafileDataStruct( counter ), new RawPacket(buffer) ) {
	}

	MetafileDataPacket( SharedBuffer &buf ) 
		: WrappedPacket<MetafileDataStruct, RawPacket>( buf ) {
	}

	unsigned counter() {
		return prefix()->counter;
	}

	SharedBuffer buffer() {
		return wrappee()->buffer();
	}

protected:
	virtual SmartPtr<RawPacket> createWrappee( SharedBuffer &buffer ) {
		return new RawPacket( buffer );
	}
};


typedef TypedPacket<char,PacketBase> MetafilePacket;
typedef EmptyPacket MetafileResetPacket;
typedef EmptyPacket MetafileAckPacket;


enum MetafileKey {
	resetPacket,
	ackPacket,
	dataPacket,
};


class MetafileFactory : public TypedPacketFactory<char,PacketBase> {
 public:
	virtual SmartPtr<PacketBase> createPacket( char key, SharedBuffer &buffer ) {
		switch( key ) {
		case resetPacket: return new MetafileResetPacket();
		case ackPacket: return new MetafileAckPacket();
		case dataPacket: return new MetafileDataPacket( buffer );
		default: return 0;
		}
	}
};


/***************************************************************************/


struct MetafileCompressor::Data {
	Data( MetafileCompressor *mc )
		: boostTimerCallback( mc, MetafileCompressor::boostMetafileThread ) {
	}

	// general stuff
	MetafileChannel *channel;
	CRITICAL_SECTION cs;

	// metafile data
	SmartPtr<GlobalMem> lastMetafile;
	SmartPtr<GlobalMem> previousMetafile;
	volatile unsigned metafileCounter;
	UINT_PTR boostTimer;
	VoidCallbackAdapter4<MetafileCompressor, HWND,UINT,UINT_PTR,DWORD> boostTimerCallback;
	int minimumMetafileSize;
	int metafileDelay;	

	MetafileFactory factory;
	int timeout; // if timeout==0 then compress else wait, used for throttling
};


MetafileCompressor::MetafileCompressor( MetafileChannel *channel, int delay ) {
	d = new Data( this );
	d->channel = channel;
	d->metafileCounter = 0;
	d->minimumMetafileSize = -1;
	d->metafileDelay = delay;
	d->timeout = 0;

	InitializeCriticalSection( &d->cs );

	// setup thread
	setPriority( THREAD_PRIORITY_BELOW_NORMAL );
	startThread( 0 );
}


MetafileCompressor::~MetafileCompressor() {
	// shutdown thread
	stopThread();

	DeleteCriticalSection( &d->cs );
	delete d;
}


void MetafileCompressor::receive( SmartPtr<PacketBase> packet ) {
	SmartPtr<MetafilePacket> mp = (MetafilePacket*)&*packet;
	switch( mp->key() ) {
	case resetPacket: 
		dout << "channel " << d->channel->channelNum()
			 << " reset metafile received" << std::endl;
		EnterCriticalSection( &d->cs );
		d->metafileCounter = 0;
		//d->previousMetafile = 0;
		LeaveCriticalSection( &d->cs );
		break;
	case ackPacket:
		EnterCriticalSection( &d->cs );
		d->timeout = 0;
		LeaveCriticalSection( &d->cs );
		break;
	default: break;
	}
}


bool MetafileCompressor::open() {
	EnterCriticalSection( &d->cs );

	// is compressor ready?
	bool ready = d->lastMetafile.isNull();
	if( !ready ) {
		LeaveCriticalSection( &d->cs );	
		return false;
	}	   

	return true;
}


void MetafileCompressor::close( SmartPtr<GlobalMem> metafile ) {
	if( !metafile.isNull() )
		d->lastMetafile = metafile;
	LeaveCriticalSection( &d->cs );	
}


int MetafileCompressor::zd_compress1(const Bytef *ref, uLong rsize,
								  const Bytef *tar, uLong tsize,
								  Bytef **delta, uLongf *dsize) {
  int rval;
  zd_stream strm;
  zd_mem_buffer dbuf;

  /* the zstream output buffer should have size greater than zero try to
   * guess buffer size, such that no memory realocation will be needed 
   */ 
  if(!(*dsize)) *dsize = tsize/4 + 64; /* *dsize should not be 0*/

  /* init io buffers */
  strm.base[0]  = (Bytef*) ref;
  strm.base_avail[0] = rsize;
  strm.base_out[0] = 0;
  strm.refnum      = 1;

  strm.next_in  = (Bytef*) tar;
  strm.total_in = 0;
  strm.avail_in = tsize;

  /* allocate the output buffer */
  zd_alloc(&dbuf, *dsize);	
  
  strm.next_out  = dbuf.pos;
  strm.total_out = 0;
  strm.avail_out = *dsize; 

  strm.zalloc = (alloc_func)0;
  strm.zfree = (free_func)0;
  strm.opaque = (voidpf)0;

  /* init huffman coder */
  rval = zd_deflateInit(&strm, ZD_DEFAULT_COMPRESSION);
  if (rval != ZD_OK)
  {
    fprintf(stderr, "%s error: %d\n", "deflateInit", rval);
    return rval;
  }

  /* compress the data */
  while((rval = zd_deflate(&strm,ZD_FINISH))==ZD_OK){
	  /* set correctly the mem_buffef internal pointer */
	  dbuf.pos = strm.next_out; 
	  
	  /* allocate more memory */
	  zd_realloc(&dbuf,dbuf.size);
	  
	  /* restore zstream internal pointer */
	  strm.next_out = (unsigned char*)dbuf.pos;
	  strm.avail_out = dbuf.size - strm.total_out;
  }
	  
  /* set correcty the mem_buffer pointers */
  dbuf.pos = strm.next_out; 

  if(rval != ZD_STREAM_END){
    fprintf(stderr, "%s error: %d\n", "deflateInit", rval);
    zd_free(&dbuf);
    zd_deflateEnd(&strm);
    return rval;
  }

  *delta = dbuf.buffer;
  *dsize = (uLong) strm.total_out;

  /* release memory */
  return zd_deflateEnd(&strm);
}


unsigned MetafileCompressor::threadProc( void *param ) {
	setPriority( THREAD_PRIORITY_ABOVE_NORMAL );

	while( true ) {
		// new work available?
		EnterCriticalSection( &d->cs );

		// already time again?
		if( d->timeout>0 )
			d->timeout--;
		else if( !d->lastMetafile.isNull() ) {
			// install boost timer
			d->boostTimer = SetTimer( NULL, 
									  NULL, 
									  d->metafileDelay, 
									  d->boostTimerCallback.callback() );
			
			// save current buffers
			SmartPtr<GlobalMem> last = d->lastMetafile;		 
			SmartPtr<GlobalMem> previous = d->previousMetafile;
			unsigned counter = d->metafileCounter;
			
			// which reference for delta compression?
			void *delta = 0;
			unsigned long deltaSize = 0;
			void *ref;
			unsigned refSize;
			if( counter==0 ) {
				// no reference, use empty 1000 byte reference
				refSize = 1000;
				ref = malloc( refSize );			
				ZeroMemory( ref, refSize );
				dout << "channel " << d->channel->channelNum() 
					 << " start fresh metafile" << std::endl;
			} else {
				if( previous.isNull() ) {
					// this really shoundn't happend!
					dout << "previous=0" << std::endl;
					KillTimer( NULL, d->boostTimer );
					d->boostTimer = 0;
					LeaveCriticalSection( &d->cs );
					break;
				}
				// use previous metafile as reference
				ref = previous->lock();
				refSize = previous->size();
			}
			
			// delta compress metafile
			setPriority( THREAD_PRIORITY_BELOW_NORMAL );
			LeaveCriticalSection( &d->cs );
			double start = MulticrewCore::multicrewCore()->time();
			int ret = zd_compress1( (const Bytef*)ref, refSize,
									(const Bytef*)last->lock(), last->size(),
									(Bytef**)&delta, &deltaSize );
			double end = MulticrewCore::multicrewCore()->time();
			/*dout << name() << " compress in " << (end-start)*1000 << "ms"
			  << " size=" << last->size()
			  << " delta=" << deltaSize << std::endl;*/
			EnterCriticalSection( &d->cs );
			setPriority( THREAD_PRIORITY_ABOVE_NORMAL );
			
            // unlock and free stuff
			last->unlock();
			if( counter==0 ) free( ref ); else previous->unlock();			   
			d->lastMetafile = 0;
			
			// compression successful?
			if( ret==ZD_OK && counter==d->metafileCounter ) {
				// send metafile
				d->channel->send( 
					true,
					new MetafilePacket( 
						dataPacket,
						new MetafileDataPacket(
							d->metafileCounter,
							SharedBuffer(new Buffer( delta, deltaSize, false )))),
					true, // safe
					lowPriority );
				
				// packet sent
				d->metafileCounter++;
				
				// save last as reference for the next one
				d->previousMetafile = last;
			} else {
				free( delta );
				dout << "channel " << d->channel->channelNum()
					 << " compress failed" << std::endl;		   
			}
			
			// done -> kill boost timer
			KillTimer( NULL, d->boostTimer );
			d->boostTimer = 0;

			// start timeout cycle
			d->timeout = 10;
		}
		
		LeaveCriticalSection( &d->cs );		
		
		// exit thread?
		if( shouldExit( d->metafileDelay ) )
			break;
	}
	
	return 0;
}


void MetafileCompressor::boostMetafileThread( HWND, UINT, UINT_PTR, DWORD) {
	EnterCriticalSection( &d->cs );
	setPriority( THREAD_PRIORITY_ABOVE_NORMAL );
	KillTimer( NULL, d->boostTimer );
	LeaveCriticalSection( &d->cs );
}


SmartPtr<PacketBase> MetafileCompressor::createPacket( SharedBuffer &buffer ) {
	return new MetafilePacket( buffer, &d->factory );
}


/********************************************************************/


struct MetafileDecompressor::Data {
	Data( MetafileDecompressor *mc ) {
	}

	// general stuff
	MetafileChannel *channel;
	CRITICAL_SECTION cs;

	SmartPtr<GlobalMem> lastMetafile;
	int metafileCounter;

	MetafileFactory factory;
};


MetafileDecompressor::MetafileDecompressor( MetafileChannel *channel ) {
	d = new Data( this );
	d->channel = channel;
	d->metafileCounter = 0;

	InitializeCriticalSection( &d->cs );
}


MetafileDecompressor::~MetafileDecompressor() {
	DeleteCriticalSection( &d->cs );
	delete d;
}


SmartPtr<GlobalMem> MetafileDecompressor::lastMetafile() {
	EnterCriticalSection( &d->cs );
	SmartPtr<GlobalMem> ret = d->lastMetafile;
	LeaveCriticalSection( &d->cs );
	return ret;
}


int MetafileDecompressor::counter() {
	return d->metafileCounter;
}


void MetafileDecompressor::receive( SmartPtr<PacketBase> packet ) {
	SmartPtr<MetafilePacket> mp = (MetafilePacket*)&*packet;
	SmartPtr<MetafileDataPacket> mdp = (MetafileDataPacket*)&*mp->wrappee();

	EnterCriticalSection( &d->cs );
	dout << "channel " << d->channel->channelNum()
		 << " metafile packet with size " << mdp->buffer().size() 
		 << " counter=" << mdp->counter() << std::endl;

	// correct sequence?
	if( d->metafileCounter!=mdp->counter() ) {
		d->metafileCounter = 0;
		d->channel->send(
			false,
			new MetafilePacket(
				resetPacket,
				new MetafileResetPacket()),
			true,
			highPriority );			
		
		dout << "channel " << d->channel->channelNum()
			 << " invalid metafile sequence, reset" << std::endl;
	} else {
		// acknowledge
		d->channel->send(
			false,
			new MetafilePacket(
				ackPacket,
				new MetafileAckPacket()),
			true,
			highPriority );			

		// decompress
		void *ref = 0;
		unsigned refSize = 0;
		SmartPtr<GlobalMem> previous;
		if( d->metafileCounter==0 ) {
			// use 1000 zeros as reference
			refSize = 1000;
			ref = malloc( refSize );
			ZeroMemory( ref, refSize );
			dout << "channel " << d->channel->channelNum()
				 << " start with fresh metafile" << std::endl;
		} else {
			// use previous as reference
			previous = d->lastMetafile;
			ref = previous->lock();
			refSize = previous->size();
		}
		
		// uncompress
		Bytef *tar = 0;
		unsigned long tarSize = 0;
		if( zd_uncompress1( (const Bytef*)ref, refSize,
							&tar, &tarSize,
							(const Bytef*)mdp->buffer().data(), 
							mdp->buffer().size() )==ZD_OK ) {				
			if( previous.isNull() )
				free( ref );
			else
				previous->unlock();
			d->metafileCounter++;				
			d->lastMetafile = new GlobalMem( tar, tarSize );
		} else {
			if( previous.isNull() )
				free( ref );				
			else
				previous->unlock();
			dout << "channel " << d->channel->channelNum()
				 << " uncompress failed" << std::endl;
		}
		
		// unlock and free stuff
		free( tar );
	}
	
	LeaveCriticalSection( &d->cs );
}


SmartPtr<PacketBase> MetafileDecompressor::createPacket( SharedBuffer &buffer ) {
	return new MetafilePacket( buffer, &d->factory );
}
