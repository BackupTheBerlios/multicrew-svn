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
#include <Gdiplus.h>
#include <stack>
#include <map>
#include <set>

#include "gauges.h"
#include "multicrewgauge.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/log.h"

extern "C" {
#include "zdelta/zdlib.h"
#include "zdelta/zd_mem.h"
}

using namespace Gdiplus;


#define METAFILEDELAY 300
#define BOOSTTIME 300


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


enum {
	mousePacket = 0,
	elementPacket,
	metafilePacket,
	metafileResetPacket
};


struct MouseStruct {
	int mouseRectNum;
	PIXPOINT pix;
	FLAGS32 flags;
};


struct MetafileResetStruct {
};


struct MetafileStruct {
	MetafileStruct( unsigned counter ) {
		this->counter = counter;
	}

	unsigned counter;
};


typedef TypedPacket<char, Packet> GaugePacket;
typedef TypedPacket<unsigned, Packet> ElementPacket;
typedef StructPacket<MouseStruct> MousePacket;
typedef StructPacket<MetafileResetStruct> MetafileResetPacket;

class MetafilePacket : public WrappedPacket<MetafileStruct, RawPacket> {
public:
	MetafilePacket( unsigned counter, SharedBuffer &buffer )
		: WrappedPacket<MetafileStruct, RawPacket>(
			MetafileStruct( counter ), new RawPacket(buffer) ) {
	}

	MetafilePacket( SharedBuffer &buf ) 
		: WrappedPacket<MetafileStruct, RawPacket>( buf ) {
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


class ElementPacketFactory : public TypedPacketFactory<unsigned,Packet> {
public:
	ElementPacketFactory( std::deque<Element *> &elements ) 
		: _elements( elements ) {
	}

	virtual SmartPtr<Packet> createPacket( unsigned key, SharedBuffer &buffer ) {
		if( key<_elements.size() )
			return _elements[key]->createPacket( buffer );
		else
			return 0;
	}

private:
	std::deque<Element *> &_elements;
};


class GaugePacketFactory : public TypedPacketFactory<char,Packet> {
public:
	GaugePacketFactory( ElementPacketFactory& elementFactory ) 
		: _elementFactory( elementFactory ) {
	}

	virtual SmartPtr<Packet> createPacket( char key, SharedBuffer &buffer ) {
		switch( key ) {
		case mousePacket: return new MousePacket( buffer );
		case elementPacket:	return new ElementPacket( buffer, &_elementFactory );
		case metafilePacket: return new MetafilePacket( buffer );
		case metafileResetPacket: return new MetafileResetPacket( buffer );
		default: return 0;
		}
	}

private:
	ElementPacketFactory& _elementFactory;
};


/*********************************************************************/


struct Gauge::Data {
	Data( Gauge *gauge )
		: callbackAdapter( gauge, Gauge::callback ),
		elementPacketFactory( elements ),
		packetFactory( elementPacketFactory ),
		boostTimerCallback( gauge, Gauge::boostMetafileThread ) {
	}

	PGAUGEHDR gaugeHeader;
	PGAUGE_CALLBACK originalGaugeCallback;
	VoidCallbackAdapter3<Gauge, PGAUGEHDR, SINT32, UINT32> callbackAdapter;
	ElementPacketFactory elementPacketFactory;
	GaugePacketFactory packetFactory;

	// mouse data
	std::deque<PMOUSE_FUNCTION> originalMouseCallbacks;
	typedef UserCallbackAdapter2<BOOL, Gauge, int, PPIXPOINT, FLAGS32> MouseCallback;
	std::deque<MouseCallback*> mouseCallbackAdapters;
	typedef SmartPtr<MousePacket> SmartMousePacket;
	std::deque<SmartMousePacket> mouseEvents;
	PMOUSERECT origMouseRect;
	bool swallowMouse;

	// metafile data
	SmartPtr<GlobalMem> lastMetafile;
	SmartPtr<GlobalMem> previousMetafile;
	int metafileElement;
	volatile unsigned metafileCounter;
	bool sendMetafileReset;
	bool resetSent;
	UINT_PTR boostTimer;
	VoidCallbackAdapter4<Gauge, HWND,UINT,UINT_PTR,DWORD> boostTimerCallback;
	int minimumMetafileSize;

	// general gauge data
	std::deque<Element *> elements;	
	std::string name;
	std::string parameters;
	int id;
	MulticrewGauge *mgauge;
	CRITICAL_SECTION cs;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;	
};


Gauge::Gauge( MulticrewGauge *mgauge, int id ) {
	d = new Data( this );
	d->mgauge = mgauge;
	d->gaugeHeader = 0;
	d->name = "";
	d->parameters = "";
	d->id = id;
	d->metafileCounter = 0;
	d->sendMetafileReset = true;
	d->resetSent = false;
	d->minimumMetafileSize = -1;

	InitializeCriticalSection( &d->cs );
}


void Gauge::createElements() {
	// create elements
	if( d->gaugeHeader->elements_list[0]!=NULL ) {
		std::stack<ELEMENT_HEADER*> todo;
		todo.push( d->gaugeHeader->elements_list[0] );
		int id = 0;
		bool verbose = 	configBoolValue( "verbose", false );
		
		while( !todo.empty() ) {
			ELEMENT_HEADER *pelement = todo.top(); todo.pop();

			EnterCriticalSection( &d->cs );

			// element not yet created?
			if( id>=d->elements.size() )
				d->elements.push_back( createElement( id, pelement ) );

			// verbose?
			if( verbose ) {
				std::string type = "unknown";
				switch( pelement->element_type ) {
				case ELEMENT_TYPE_ICON: type="icon"; break;
				case ELEMENT_TYPE_NEEDLE: type="needle"; break;
				case ELEMENT_TYPE_STRING: type="string"; break;
				case ELEMENT_TYPE_STATIC_IMAGE: type="static"; break;
				default: break;
				}

				dlog << "element " << id << " of type " << type << std::endl;
			}

			// attach element
			if( d->elements[id] ) d->elements[id]->attach( pelement );

			id++;
			LeaveCriticalSection( &d->cs );

			if( pelement->next_element!=NULL ) {
				for( int i=0; pelement->next_element[i]!=NULL; i++ )
					todo.push( pelement->next_element[i] );				
			}
		}
	}
}


Gauge::~Gauge() {
	dout << "~Gauge " << name() << " " << parameter() << std::endl;			
	EnterCriticalSection( &d->cs );

	detach();

	// delete all elements
	std::deque<Element*>::iterator it = d->elements.begin();
	while( it!=d->elements.end() ) delete *it++;
	d->elements.clear();

	LeaveCriticalSection( &d->cs );
	DeleteCriticalSection( &d->cs );
	delete d;
}


PGAUGEHDR Gauge::gaugeHeader() { 
	return d->gaugeHeader; 
}


std::string Gauge::name() {	
	return d->name;
}


std::string Gauge::parameter() {	
	return d->parameters;
}


int Gauge::id() {
	return d->id;
}


MulticrewGauge *Gauge::mgauge() {
	return d->mgauge;
}


void Gauge::send( unsigned element, SmartPtr<Packet> packet, 
				  bool safe, bool async ) {
	d->mgauge->send( id(), 
					 new GaugePacket( 
						 elementPacket,
						 new ElementPacket(element, packet)),
					 safe, Connection::mediumPriority, async );
}


void Gauge::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case elementPacket: 
	{
		SmartPtr<ElementPacket> ep = (ElementPacket*)&*gp->wrappee();
		EnterCriticalSection( &d->cs );
		if( ep->key()>=0 && ep->key()<d->elements.size() ) {
			Element *element = d->elements[ep->key()];
			LeaveCriticalSection( &d->cs );
			if( element ) element->receive( ep->wrappee() );
		} else
			LeaveCriticalSection( &d->cs );
	} break;
	case mousePacket:
		EnterCriticalSection( &d->cs );
		d->mouseEvents.push_back( (MousePacket*)&*gp->wrappee() );
		LeaveCriticalSection( &d->cs );
		break;
	default:
		break;
	}
}


SmartPtr<Packet> Gauge::createPacket( SharedBuffer &buffer ) {
	EnterCriticalSection( &d->cs );
	SmartPtr<GaugePacket> gp = new GaugePacket( buffer, &d->packetFactory );
	LeaveCriticalSection( &d->cs );
	return gp;
}


void Gauge::sendProc() {
    // call element's sendProc
	EnterCriticalSection( &d->cs );
	for( int i=0; i<d->elements.size(); i++ ) {
		Element *element = d->elements[i];
		if( element ) element->sendProc();
	}
	LeaveCriticalSection( &d->cs );
}


void Gauge::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call original callback
	if( d->originalGaugeCallback!=NULL ) 
		(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );

	//if( service_id>=12 ) {
	//	dout << d->name << " " << service_id << std::endl;
	//}

	// if a pre kill is received the plane is either closed
	// or the panel is resized. In the latter case all gauges will be
	// recreated during the next moments
	if( service_id==PANEL_SERVICE_PRE_KILL ) {
		//dout << "PANEL_SERVICE_PRE_KILL" << std::endl;
		detach();
	}
}


void Gauge::attach( PGAUGEHDR gaugeHeader ) {	
	EnterCriticalSection( &d->cs );

	// detach first if attached
	if( d->gaugeHeader ) detach();

	// initialize general variables
	d->gaugeHeader = gaugeHeader;
	d->name = gaugeHeader->gauge_name;	
	d->parameters = (gaugeHeader->parameters!=NULL)?gaugeHeader->parameters:"";
	d->swallowMouse = configBoolValue( "swallowmouse", false );
	if( d->swallowMouse ) dout << "will swallow mouse" << std::endl;

	// hook elements
	bool hookElements = configBoolValue( "hookelements", true );
	if( hookElements ) createElements();
	
	// setup callback
	d->originalGaugeCallback = d->gaugeHeader->gauge_callback;
	if( d->originalGaugeCallback!=NULL )		
		d->gaugeHeader->gauge_callback = d->callbackAdapter.callback();	

	// setup mouse callbacks
	bool hookMouse = configBoolValue( "hookmouse", true );
	d->origMouseRect = d->gaugeHeader->mouse_rect;
	if( d->gaugeHeader->mouse_rect!=0 && hookMouse ) {
		PMOUSERECT rect = d->gaugeHeader->mouse_rect;
		int num = 0;
	   	while( rect->rect_type!=MOUSE_RECT_EOL ) {
			d->originalMouseCallbacks.push_back( rect->mouse_function );
			if( rect->mouse_function!=0 || rect->event_id!=0 ) {
				//dout << "Wrapping mouse callback for " << d->name << std::endl;
				Data::MouseCallback *callback = new Data::MouseCallback( this, Gauge::mouseCallback, num );
				d->mouseCallbackAdapters.push_back( callback );
				rect->mouse_function = callback->callback();
			}

			num++;
			rect++;
		}
	}

	//dout << "gaugeHeader=" << gaugeHeader << std::endl;

	LeaveCriticalSection( &d->cs );
}


bool Gauge::configBoolValue( const std::string &key, bool def ) {
	// first look in [name!parameter], then in [name], then def
	Config *c = d->mgauge->config();
	return c->boolValue( name()+"!"+parameter(), key,
						 c->boolValue( name(), key, def ) );
}


int Gauge::configIntValue( const std::string &key, int def ) {
	// first look in [name!parameter], then in [name], then def
	Config *c = d->mgauge->config();
	return c->intValue( name()+"!"+parameter(), key,
						c->intValue( name(), key, def ) );
}


void Gauge::detach() {
	if( d->gaugeHeader ) {
		EnterCriticalSection( &d->cs );

		// restore gauge callback
		d->gaugeHeader->gauge_callback = d->originalGaugeCallback;

		// restore mouse callbacks
		if( d->gaugeHeader->mouse_rect!=0 && d->origMouseRect==d->gaugeHeader->mouse_rect ) {
			PMOUSERECT rect = d->gaugeHeader->mouse_rect;
			int num = 0;
			while( rect->rect_type!=MOUSE_RECT_EOL ) {
				rect->mouse_function = d->originalMouseCallbacks[num];
				num++;
				rect++;
			}
		}	

		// detach all elements
		std::deque<Element*>::iterator it = d->elements.begin();
		while( it!=d->elements.end() ) {
			Element *element = *(it++);
			if( element ) element->detach();
		}
										   
		// delete mouse callbacks
		std::deque<Data::MouseCallback*>::iterator it2 = d->mouseCallbackAdapters.begin();
		while( it2!=d->mouseCallbackAdapters.end() ) delete *it2++;
		d->mouseCallbackAdapters.clear();
		d->originalMouseCallbacks.clear();

		d->mgauge->detached( this );
		d->gaugeHeader = 0;
		LeaveCriticalSection( &d->cs );
	}
}


/********************************************************************************************/


GaugeRecorder::GaugeRecorder( MulticrewGauge *mgauge, int id ) 
	: Gauge( mgauge, id ) {
}


GaugeRecorder::~GaugeRecorder() {
}


Element *GaugeRecorder::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;
	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: el = new IconRecorder( id, *this ); break;
	case ELEMENT_TYPE_NEEDLE: el = new NeedleRecorder( id, *this ); break;
	case ELEMENT_TYPE_STRING: el = new StringRecorder( id, *this ); break;
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticRecorder( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeRecorder::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_PRE_DRAW: {
		// handle mouse events which came in from the network
		EnterCriticalSection( &d->cs );
		for( std::deque<Data::SmartMousePacket>::iterator it = d->mouseEvents.begin();
			 it!=d->mouseEvents.end();
			 it++ ) {
			// emit mouse event
			dout << "mouse packet for " << d->name << " mouserect " << (*it)->data().mouseRectNum << std::endl;
			
			if( (*it)->data().mouseRectNum>=0 && (*it)->data().mouseRectNum<d->originalMouseCallbacks.size() ) {
				PMOUSE_FUNCTION cb = d->originalMouseCallbacks[(*it)->data().mouseRectNum];
				PMOUSERECT rect = d->gaugeHeader->mouse_rect + (*it)->data().mouseRectNum;
				
				// has callback?
				if( cb!=0 ) {
					// call callback
					dout << "Call mouse callback for " << d->name << std::endl;
					cb( &(*it)->data().pix, (*it)->data().flags );
				} else {
					// has eventid?
					if( rect->event_id!=0 ) {
						// trigger event
						dout << "Trigger key event " << rect->event_id << " from " << d->name << std::endl;
						trigger_key_event( rect->event_id, 0 );
					}
				}
			}
		}
		d->mouseEvents.clear();
		LeaveCriticalSection( &d->cs );
	} break;

		
	default:
		break;
	}
	
	// call inherited callback
	Gauge::callback( pgauge, service_id, extra_data );
}


void GaugeRecorder::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case mousePacket:
		EnterCriticalSection( &d->cs );
		dout << "Received mouse packet for " << d->name << std::endl;
		d->mouseEvents.push_back( (MousePacket*)&*gp->wrappee() );
		LeaveCriticalSection( &d->cs );
		break;

	default:
		Gauge::receive( packet );
		break;
	}
}


BOOL GaugeRecorder::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	EnterCriticalSection( &d->cs );
	if( !d->swallowMouse && 
		mouseRectNum>=0 && 
		mouseRectNum<d->originalMouseCallbacks.size() && 
		d->originalMouseCallbacks[mouseRectNum]!=0 ) {
		dout << "mouseCallback for " << d->name << std::endl;
		PMOUSE_FUNCTION cb = d->originalMouseCallbacks[mouseRectNum];
		cb( pix, flags );
	}
	LeaveCriticalSection( &d->cs );
	
	return TRUE;
}


/**************************************************************************/


GaugeMetafileRecorder::GaugeMetafileRecorder( MulticrewGauge *mgauge, int id, int metafileElement ) 
	: GaugeRecorder( mgauge, id ) {
	startThread( 0 );
	setPriority( THREAD_PRIORITY_BELOW_NORMAL );
	Gauge::d->metafileElement = metafileElement;
}


GaugeMetafileRecorder::~GaugeMetafileRecorder() {
	stopThread();
}


Element *GaugeMetafileRecorder::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;
	switch( pelement->element_type ) {
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticRecorder( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeMetafileRecorder::callback( PGAUGEHDR pgauge, SINT32 service_id, 
									  UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_POST_INSTALL:
		GdiplusStartup(&Gauge::d->gdiplusToken, &Gauge::d->gdiplusStartupInput, NULL);
		break;
		
	case PANEL_SERVICE_PRE_DRAW: {
		// record metafile for gauge output
		EnterCriticalSection( &Gauge::d->cs );
		if( Gauge::d->elements.size()>Gauge::d->metafileElement && 
			Gauge::d->elements[Gauge::d->metafileElement] && 
			Gauge::d->lastMetafile.isNull() ) {
			Element *element = Gauge::d->elements[Gauge::d->metafileElement];
			PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();
		
			// create memory stream
			IStream *stream;
			CreateStreamOnHGlobal( NULL, FALSE, &stream );
		 	
			// create metafile
			RectF bounds( 0, 0, 
						  staticImage->image_data.final->dim.x,
						  staticImage->image_data.final->dim.y );
			Metafile metafile( stream, staticImage->hdc, 
							   bounds, MetafileFrameUnitPixel,
							   EmfTypeEmfPlusOnly );
			
			// let gauge draw to metafile
			HDC origHdc = staticImage->hdc;
			Graphics *metaGraphics = new Graphics( &metafile );
			metaGraphics->SetSmoothingMode( SmoothingModeAntiAlias );
			staticImage->hdc = metaGraphics->GetHDC();				
			(*Gauge::d->originalGaugeCallback)( Gauge::d->gaugeHeader, service_id, extra_data );
			metaGraphics->ReleaseHDC( staticImage->hdc );
			delete metaGraphics;
			staticImage->hdc = origHdc;
			origHdc = (HDC)0x204c5047;
			
			// display metafile
			/*Graphics graphics( staticImage->hdc );
			graphics.SetSmoothingMode( SmoothingModeAntiAlias );
			graphics.DrawImage( &metafile, 0, 0 );*/
			
			// save metafile for later sending
			HGLOBAL hmem;
			GetHGlobalFromStream( stream, &hmem );
			stream->Release();
			if( Gauge::d->minimumMetafileSize<0 )
				Gauge::d->minimumMetafileSize = configIntValue( "metafileminimum", 500 );
			if( GlobalSize(hmem)>Gauge::d->minimumMetafileSize ) {
				Gauge::d->lastMetafile = new GlobalMem(hmem);
			} else
				GlobalFree( hmem );
				 
			LeaveCriticalSection( &Gauge::d->cs );
			return;
		}				
		LeaveCriticalSection( &Gauge::d->cs );
	} break;

	case PANEL_SERVICE_DISCONNECT:
		GdiplusShutdown(Gauge::d->gdiplusToken);
		break;
		
	default:
		break;
	}
	
	// call inherited callback
	GaugeRecorder::callback( pgauge, service_id, extra_data );
}


void GaugeMetafileRecorder::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case metafileResetPacket:
	{
		dout << name() << " reset metafile received" << std::endl;
		EnterCriticalSection( &Gauge::d->cs );
		Gauge::d->metafileCounter = 0;
		//Gauge::d->previousMetafile = 0;
		LeaveCriticalSection( &Gauge::d->cs );
	} break;

	default:
		GaugeRecorder::receive( packet );
		break;
	}
}


int GaugeMetafileRecorder::zd_compress1(const Bytef *ref, uLong rsize,
			 const Bytef *tar, uLong tsize,
			 Bytef **delta, uLongf *dsize)
{
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


unsigned GaugeMetafileRecorder::threadProc( void *param ) {
	setPriority( THREAD_PRIORITY_ABOVE_NORMAL );

	while( true ) {
		// new work available?
		EnterCriticalSection( &Gauge::d->cs );
		if( !Gauge::d->lastMetafile.isNull() ) {
			// install boost timer
			Gauge::d->boostTimer = SetTimer( NULL, 
											 NULL, 
											 BOOSTTIME, 
											 Gauge::d->boostTimerCallback.callback() );

			// save current buffers
			SmartPtr<GlobalMem> last = Gauge::d->lastMetafile;		 
			SmartPtr<GlobalMem> previous = Gauge::d->previousMetafile;
			unsigned counter = Gauge::d->metafileCounter;

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
				dout << name() << " start fresh metafile" << std::endl;
			} else {
				if( previous.isNull() ) {
					// this really shoundn't happend!
					dout << "previous=0" << std::endl;
					KillTimer( NULL, Gauge::d->boostTimer );
					Gauge::d->boostTimer = 0;
					LeaveCriticalSection( &Gauge::d->cs );
					break;
				}
				// use previous metafile as reference
				ref = previous->lock();
				refSize = previous->size();
			}
								
			// delta compress metafile
			setPriority( THREAD_PRIORITY_BELOW_NORMAL );
			LeaveCriticalSection( &Gauge::d->cs );
			double start = MulticrewCore::multicrewCore()->time();
			int ret = zd_compress1( (const Bytef*)ref, refSize,
									(const Bytef*)last->lock(), last->size(),
									(Bytef**)&delta, &deltaSize );
			double end = MulticrewCore::multicrewCore()->time();
			dout << name() << " compress in " << (end-start)*1000 << "ms"
				 << " size=" << last->size()
				 << " delta=" << deltaSize << std::endl;
			EnterCriticalSection( &Gauge::d->cs );
			setPriority( THREAD_PRIORITY_ABOVE_NORMAL );

            // unlock and free stuff
			last->unlock();
			if( counter==0 ) free( ref ); else previous->unlock();			   
			Gauge::d->lastMetafile = 0;
			
			// compression successful?
			if( ret==ZD_OK && counter==Gauge::d->metafileCounter ) {
				// send metafile
				Gauge::d->mgauge->send( 
					id(), 
					new GaugePacket( 
						metafilePacket,
						new MetafilePacket( 
							Gauge::d->metafileCounter,
							SharedBuffer(new Buffer( delta, deltaSize, false )))),
					true, // safe
					Connection::lowPriority,
					true ); // async
		
				// packet sent
				Gauge::d->metafileCounter++;
			
				// save last as reference for the next one
				Gauge::d->previousMetafile = last;
			} else {
				free( delta );
				dout << name() << " compress failed" << std::endl;		   
			}

			// done -> kill boost timer
			KillTimer( NULL, Gauge::d->boostTimer );
			Gauge::d->boostTimer = 0;
		}

		LeaveCriticalSection( &Gauge::d->cs );		
	
		// exit thread?
		if( shouldExit( METAFILEDELAY ) )
			break;
	}

	return 0;
}


void GaugeMetafileRecorder::boostMetafileThread( HWND, UINT, UINT_PTR, DWORD) {
	EnterCriticalSection( &Gauge::d->cs );
	setPriority( THREAD_PRIORITY_ABOVE_NORMAL );
	KillTimer( NULL, Gauge::d->boostTimer );
	LeaveCriticalSection( &Gauge::d->cs );
}


/**************************************************************************/


GaugeViewer::GaugeViewer( MulticrewGauge *mgauge, int id ) 
	: Gauge( mgauge, id ) {
}


GaugeViewer::~GaugeViewer() {
}


Element *GaugeViewer::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;

	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: el = new IconViewer( id, *this ); break;	
	case ELEMENT_TYPE_NEEDLE: el = new NeedleViewer( id, *this ); break;
	case ELEMENT_TYPE_STRING: el = new StringViewer( id, *this ); break;
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticViewer( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeViewer::sendProc() {
	Gauge::sendProc();

	// send mouse events
	EnterCriticalSection( &d->cs );
	std::deque<Data::SmartMousePacket>::iterator it = d->mouseEvents.begin();
	while( it!=d->mouseEvents.end() ) {
		d->mgauge->send( 
			id(), 
			new GaugePacket( 
				mousePacket,
				&*(*it)), 
			false,
			Connection::highPriority );		
		it++;
	}
	d->mouseEvents.clear();
	LeaveCriticalSection( &d->cs );
}


BOOL GaugeViewer::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
	dout << "mouseCallback for " << d->name << std::endl;

	// append mouse event to d->mouseEvents which is sent during the 
    // next sendProc call
	EnterCriticalSection( &d->cs );
	if( !d->swallowMouse ) {
		while( d->mouseEvents.size()>5 ) {
			// don't let the queue get too full
			d->mouseEvents.pop_front();
		}
		MouseStruct s;
		s.mouseRectNum = mouseRectNum;
		memcpy( &s.pix, pix, sizeof(PIXPOINT) );
		s.flags = flags;
		d->mouseEvents.push_back( new MousePacket( s ) );
	}
	LeaveCriticalSection( &d->cs );
	
	return TRUE;
}


/**************************************************************************/


GaugeMetafileViewer::GaugeMetafileViewer( MulticrewGauge *mgauge, int id, int metafileElement ) 
	: GaugeViewer( mgauge, id ) {
	d->metafileElement = metafileElement;
}


GaugeMetafileViewer::~GaugeMetafileViewer() {
}


void GaugeMetafileViewer::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_POST_INSTALL:
		GdiplusStartup(&d->gdiplusToken, &d->gdiplusStartupInput, NULL);
		break;
		
	case PANEL_SERVICE_PRE_DRAW: {
		// display
		EnterCriticalSection( &d->cs );
		if( d->elements.size()>d->metafileElement && 
			d->elements[d->metafileElement] && 
			!d->lastMetafile.isNull() ) {
			dout << "draw metafile" << std::endl;

			Element *element = d->elements[d->metafileElement];
			PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();
			PPIXPOINT dim = &staticImage->image_data.final->dim;
			
			// create stream
			IStream *stream;
			CreateStreamOnHGlobal( d->lastMetafile->handle(), FALSE, &stream );			
			Metafile metafile( stream );
					
			// display metafile		
			Graphics graphics( staticImage->hdc );
			Color black;
			graphics.Clear( black );
			graphics.SetSmoothingMode( SmoothingModeAntiAlias );
			graphics.SetTextRenderingHint( TextRenderingHintAntiAlias );
			graphics.DrawImage( &metafile, 0, 0,
								dim->x, dim->y );			
			stream->Release();
			d->previousMetafile = d->lastMetafile;
			d->lastMetafile = 0;
			SET_OFF_SCREEN( staticImage );
		}
		LeaveCriticalSection( &d->cs );
		return;
	} break;

	case PANEL_SERVICE_DISCONNECT:
		GdiplusShutdown(d->gdiplusToken);
		break;
		
	default:
		break;
	}

	GaugeViewer::callback( pgauge, service_id, extra_data );
}

Element *GaugeMetafileViewer::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;

	switch( pelement->element_type ) {
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticViewer( id, *this ); break;
	default: break;
	}

	return el;
}


void GaugeMetafileViewer::sendProc() {
	GaugeViewer::sendProc();

	// metafile reset?
	EnterCriticalSection( &d->cs );
	if( d->sendMetafileReset ) {
		dout << name() << " sending reset metafile" << std::endl;
		d->sendMetafileReset = false;
		d->mgauge->send(
			id(),
			new GaugePacket(
				metafileResetPacket,
				new MetafileResetPacket( MetafileResetStruct() )),
			true,
			Connection::highPriority );
	}	
	LeaveCriticalSection( &d->cs );
}


void GaugeMetafileViewer::receive( SmartPtr<Packet> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case metafilePacket:
	{
		EnterCriticalSection( &d->cs );
		
		// metafile packet
		SmartPtr<MetafilePacket> mp = (MetafilePacket*)&*gp->wrappee();
		dout << this << " metafile packet with size " << mp->buffer().size() 
			 << " counter=" << mp->counter() << std::endl;

		// correct sequence?
		if( d->metafileCounter!=mp->counter() ) {
			if( !d->resetSent ) {
				d->sendMetafileReset = true;
				d->previousMetafile = 0;
				d->metafileCounter = 0;
				d->resetSent = true;
				dout << this << " invalid metafile sequence, reset" << std::endl;
			}
		} else {
			d->resetSent = false;
			void *ref = 0;
			unsigned refSize = 0;
			SmartPtr<GlobalMem> previous;
			if( d->metafileCounter==0 ) {
				// use 1000 zeros as reference
				refSize = 1000;
				ref = malloc( refSize );
				ZeroMemory( ref, refSize );
				dout << this << " start with fresh metafile" << std::endl;
			} else {
                // use previous as reference
				if( d->lastMetafile.isNull() )
					previous = d->previousMetafile;
				else
					previous = d->lastMetafile;			 
				ref = previous->lock();
				refSize = previous->size();
			}

			// uncompress
			Bytef *tar = 0;
			unsigned long tarSize = 0;
			if( zd_uncompress1( (const Bytef*)ref, refSize,
								&tar, &tarSize,
								(const Bytef*)mp->buffer().data(), 
								mp->buffer().size() )==ZD_OK ) {				
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
				dout << this << " uncompress failed" << std::endl;
			}

			// unlock and free stuff
			free( tar );
		}
			
		LeaveCriticalSection( &d->cs );
	} break;

	default:
		Gauge::receive( packet );
		break;
	}
}
