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
#include "metafile.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/callback.h"
#include "../multicrewcore/log.h"

using namespace Gdiplus;


/********************************** packets *******************************/
enum {
	mousePacket = 0,
	elementPacket,
};


struct MouseStruct {
	int mouseRectNum;
	float relX;
	float relY;
	float scrX;
	float scrY;
	FLAGS32 flags;
};


typedef TypedPacket<char, PacketBase> GaugePacket;
typedef TypedPacket<unsigned, PacketBase> ElementPacket;
typedef StructPacket<MouseStruct> MousePacket;


class ElementPacketFactory : public TypedPacketFactory<unsigned,PacketBase> {
public:
	ElementPacketFactory( std::deque<Element *> &elements ) 
		: _elements( elements ) {
	}

	virtual SmartPtr<PacketBase> createPacket( unsigned key, SharedBuffer &buffer ) {
		if( key<_elements.size() )
			return _elements[key]->createPacket( buffer );
		else
			return 0;
	}

private:
	std::deque<Element *> &_elements;
};


class GaugePacketFactory : public TypedPacketFactory<char,PacketBase> {
public:
	GaugePacketFactory( ElementPacketFactory& elementFactory ) 
		: _elementFactory( elementFactory ) {
	}

	virtual SmartPtr<PacketBase> createPacket( char key, SharedBuffer &buffer ) {
		switch( key ) {
		case mousePacket: return new MousePacket( buffer );
		case elementPacket:	return new ElementPacket( buffer, &_elementFactory );
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
		  packetFactory( elementPacketFactory ) {
	}

	PGAUGEHDR gaugeHeader;
	PGAUGE_CALLBACK originalGaugeCallback;
	VoidCallbackAdapter3<Gauge, PGAUGEHDR, SINT32, UINT32> callbackAdapter;
	ElementPacketFactory elementPacketFactory;
	GaugePacketFactory packetFactory;
	SmartPtr<MulticrewCore> core;

	// mouse data
	bool hookMouse;
	std::deque<PMOUSE_FUNCTION> originalMouseCallbacks;
	typedef UserCallbackAdapter2<BOOL, Gauge, int, PPIXPOINT, FLAGS32> MouseCallback;
	std::deque<MouseCallback*> mouseCallbackAdapters;
	typedef SmartPtr<MousePacket> SmartMousePacket;
	std::deque<SmartMousePacket> mouseEvents;
	PMOUSERECT origMouseRect;
	bool swallowMouse;

	// general gauge data
	std::deque<Element *> elements;	
	std::set<Element*> sendRequests;
	std::string name;
	std::string parameters;
	int id;
	GaugeModule *mgauge;
	CRITICAL_SECTION cs;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;	
};


Gauge::Gauge( GaugeModule *mgauge, int id ) {
	d = new Data( this );
	d->mgauge = mgauge;
	d->gaugeHeader = 0;
	d->name = "";
	d->parameters = "";
	d->id = id;
	d->core = MulticrewCore::multicrewCore();

	InitializeCriticalSection( &d->cs );
}


void Gauge::createElements() {
	// create elements
	if( d->gaugeHeader->elements_list[0]!=NULL ) {
		std::stack<ELEMENT_HEADER*> todo;
		todo.push( d->gaugeHeader->elements_list[0] );
		int id = 0;
		bool verbose = configBoolValue( "verbose", false );
		
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
				
				dout << "gauge " << d->name << " element " << id << " of type " << type << std::endl;
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


GaugeModule *Gauge::mgauge() {
	return d->mgauge;
}


void Gauge::send( unsigned element, SmartPtr<PacketBase> packet, 
				  bool safe, bool async ) {
	d->mgauge->send( id(), 
					 new GaugePacket( 
						 elementPacket,
						 new ElementPacket(element, packet)),
					 safe, Connection::mediumPriority, async );
}


void Gauge::receive( SmartPtr<PacketBase> packet ) {
	SmartPtr<GaugePacket> gp = (GaugePacket*)&*packet;
	switch( gp->key() ) {
	case elementPacket: 
	{
		SmartPtr<ElementPacket> ep = (ElementPacket*)&*gp->wrappee();
		dout << d->name << " received element packet for " << ep->key() << std::endl;
		EnterCriticalSection( &d->cs );
		if( ep->key()>=0 && ep->key()<d->elements.size() ) {
			Element *element = d->elements[ep->key()];
			LeaveCriticalSection( &d->cs );
			if( element ) element->receive( ep->wrappee() );
		} else
			LeaveCriticalSection( &d->cs );
	} break;
	case mousePacket:
		dout << "received mouse packet" << std::endl;
		EnterCriticalSection( &d->cs );
		d->mouseEvents.push_back( (MousePacket*)&*gp->wrappee() );
		LeaveCriticalSection( &d->cs );
		handleMouseEvents();
		break;
	default:
		break;
	}
}


SmartPtr<PacketBase> Gauge::createPacket( SharedBuffer &buffer ) {
	EnterCriticalSection( &d->cs );
	SmartPtr<GaugePacket> gp = new GaugePacket( buffer, &d->packetFactory );
	LeaveCriticalSection( &d->cs );
	return gp;
}


void Gauge::requestSend( Element *element ) {
	EnterCriticalSection( &d->cs );
	d->sendRequests.insert( element );
	LeaveCriticalSection( &d->cs );
	
	d->mgauge->requestSend( this );
}


void Gauge::sendProc( bool fullSend ) {
	EnterCriticalSection( &d->cs );

    // call element's sendProc
	if( fullSend ) {
		//full send
		for( int i=0; i<d->elements.size(); i++ ) {
			Element *element = d->elements[i];
			if( element ) element->sendProc();
		}
	} else {
		// only send changed element packets
		for( std::set<Element*>::iterator it = d->sendRequests.begin();
			 it!=d->sendRequests.end();
			 it++ )
			(*it)->sendProc();
	}
	
	// clear send queue
	d->sendRequests.clear();

	LeaveCriticalSection( &d->cs );
}


void Gauge::callback( PGAUGEHDR pgauge, SINT32 service_id, UINT32 extra_data ) {
	// call original callback
	if( d->originalGaugeCallback!=NULL ) 
		(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );

	/*if( service_id>=12 || service_id<=5 ) {
		dout << d->name << " " << service_id << std::endl;
		}*/

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
	d->gaugeHeader->gauge_callback = d->callbackAdapter.callback();	

	// setup mouse callbacks
	d->hookMouse = configBoolValue( "hookmouse", true );
	d->origMouseRect = d->gaugeHeader->mouse_rect;
	if( d->gaugeHeader->mouse_rect!=0 && d->hookMouse ) {
		PMOUSERECT rect = d->gaugeHeader->mouse_rect;
		int num = 0;
		if( rect->rect_type==MOUSE_RECT_EOL ) dout << "No mouse for " << d->name << std::endl;
	   	while( rect->rect_type!=MOUSE_RECT_EOL ) {
            dout << "Wrapping mouse callback for " << d->name << std::endl;
			d->originalMouseCallbacks.push_back( rect->mouse_function );			
			Data::MouseCallback *callback = 
				new Data::MouseCallback( this, Gauge::mouseCallback, num );
			d->mouseCallbackAdapters.push_back( callback );
			rect->mouse_function = callback->callback();

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
		if( d->gaugeHeader->mouse_rect!=0 && 
			d->origMouseRect==d->gaugeHeader->mouse_rect &&
			d->hookMouse ) {
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


Element *Gauge::createElement( int id, PELEMENT_HEADER pelement ) {
	Element *el = 0;

	switch( pelement->element_type ) {
	case ELEMENT_TYPE_ICON: el = new IconElement( id, *this ); break;	
	case ELEMENT_TYPE_NEEDLE: el = new NeedleElement( id, *this ); break;
	case ELEMENT_TYPE_STRING: el = new StringElement( id, *this ); break;
	case ELEMENT_TYPE_STATIC_IMAGE: el = new StaticElement( id, *this ); break;
	default: break;
	}

	return el;
}


BOOL Gauge::mouseCallback( int mouseRectNum, PPIXPOINT pix, FLAGS32 flags ) {
    // the pix is a MOUSECALLBACK structure in fact. Look into GAUGES.H.
	// that's Microsoft's coding practise ;-)
	PMOUSECALLBACK mc = (PMOUSECALLBACK)pix;
			
	dout << "mouseCallback "
		 << d->name
		 << " rel=" << mc->relative_point.x
		 << ":" << mc->relative_point.y
		 << " screen=" << mc->screen_point.x
		 << ":" << mc->screen_point.y
		 << " rect=" << mouseRectNum
		 << std::endl;

	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: {
		if( mouseRectNum<d->originalMouseCallbacks.size() && 
			d->originalMouseCallbacks[mouseRectNum]!=0 )
			return d->originalMouseCallbacks[mouseRectNum]( pix, flags );
	} break;
	case MulticrewCore::HostMode: {
		EnterCriticalSection( &d->cs );
		if( !d->swallowMouse && 
			mouseRectNum>=0 &&
			mouseRectNum<d->originalMouseCallbacks.size() && 
			d->originalMouseCallbacks[mouseRectNum]!=0 ) {
			PMOUSE_FUNCTION cb = d->originalMouseCallbacks[mouseRectNum];
			cb( pix, flags );
		}
		LeaveCriticalSection( &d->cs );
		
		return TRUE;
	} break;
	case MulticrewCore::ClientMode: {
		// append mouse event to d->mouseEvents which is sent during the 
		// next sendProc call
		EnterCriticalSection( &d->cs );
		if( !d->swallowMouse ) {
			// create MouseStruct data
			MouseStruct ms;
			ms.mouseRectNum = mouseRectNum;
			ms.relX = mc->relative_point.x*1.0f/d->gaugeHeader->size.x;
			ms.relY = mc->relative_point.y*1.0f/d->gaugeHeader->size.y;
			ms.scrX = mc->screen_point.x*1.0f/d->gaugeHeader->size.x;
			ms.scrY = mc->screen_point.y*1.0f/d->gaugeHeader->size.y;
			ms.flags = flags;
			
			// send MouseStruct packet
			d->mgauge->send( 
				id(), 
				new GaugePacket(
					mousePacket,
					new MousePacket(ms)), 
				false, // safe
				Connection::highPriority,
				true ); // async		
		}
		LeaveCriticalSection( &d->cs );
		
		return FALSE;
	} break;
	}

	return true;
}


void Gauge::handleMouseEvents() {
	EnterCriticalSection( &d->cs );

	switch( d->core->mode() ) {
	case MulticrewCore::IdleMode: break;
	case MulticrewCore::HostMode: {
		// handle mouse events which came in from the network
		for( std::deque<Data::SmartMousePacket>::iterator it = d->mouseEvents.begin();
			 it!=d->mouseEvents.end();
			 it++ ) {
			MouseStruct &ms = (*it)->data();
			
			// emit mouse event
			dout << "mouse packet for " << d->name
				 << " for " << d->name
				 << " rel=" << ms.relX
				 << ":" << ms.relY
				 << " screen=" << ms.scrX
				 << ":" << ms.scrY
				 << std::endl;
						
			int num =  ms.mouseRectNum;
			if( num>=0 && num<d->originalMouseCallbacks.size() ) {
				PMOUSE_FUNCTION cb = d->originalMouseCallbacks[num];
				PMOUSERECT rect = d->gaugeHeader->mouse_rect + num;
				
				// has callback?
				if( cb!=0 ) {
					// create MOUSECALLBACK structure
					MOUSECALLBACK mc;
					mc.relative_point.x = (int)(ms.relX * d->gaugeHeader->size.x);
					mc.relative_point.y = (int)(ms.relY * d->gaugeHeader->size.y);
					mc.screen_point.x = (int)(ms.scrX * d->gaugeHeader->size.x);
					mc.screen_point.y = (int)(ms.scrY * d->gaugeHeader->size.y);
					mc.user_data = (PVOID)d->gaugeHeader;
					mc.mouse = rect;
					mc.reserved = 0;
					
					// call callback
					dout << "Call mouse callback " << (void*)cb 
						 << " for " << d->name << std::endl;
					cb( (PPIXPOINT)&mc, ms.flags );
				}
				
				// has eventid?
				if( rect->event_id!=0 ) {
					// trigger event					
					dout << "Trigger key event " << rect->event_id << " from " << d->name << std::endl;
					trigger_key_event( rect->event_id, 0 );				
				}
			}
		}
	} break;
	case MulticrewCore::ClientMode: break;
	}

	d->mouseEvents.clear();
	LeaveCriticalSection( &d->cs );
}


/**************************************************************************/
struct MetafileGauge::Data {
	int minimumMetafileSize;
	int metafileElement;
	int metafileCounter;	

	SmartPtr<MetafileCompressor> compressor;
	SmartPtr<MetafileDecompressor> decompressor;
};


MetafileGauge::MetafileGauge( GaugeModule *mgauge, int id, int metafileElement,
							  SmartPtr<MetafileCompressor> compressor,
							  SmartPtr<MetafileDecompressor> decompressor ) 
	: Gauge( mgauge, id ) {
	md = new Data;
	md->metafileElement = metafileElement;	
	md->metafileCounter = -1;
	md->compressor = compressor;
	md->decompressor = decompressor;
}


MetafileGauge::~MetafileGauge() {
	delete md;
}


void MetafileGauge::callback( PGAUGEHDR pgauge, SINT32 service_id, 
							  UINT32 extra_data ) {
	// do work of callback
	switch (service_id) {
	case PANEL_SERVICE_POST_INSTALL:
		GdiplusStartup(&d->gdiplusToken, &d->gdiplusStartupInput, NULL);
		break;
		
	case PANEL_SERVICE_PRE_DRAW: {
		switch( d->core->mode() ) {
		case MulticrewCore::IdleMode: break;
		case MulticrewCore::HostMode: {
			// record metafile for gauge output
			EnterCriticalSection( &d->cs );
			if( d->elements.size()>md->metafileElement && 
				d->elements[md->metafileElement] && 
				md->compressor->open() ) {
				Element *element = d->elements[md->metafileElement];
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
				(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );
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
				if( md->minimumMetafileSize<0 )
					md->minimumMetafileSize = configIntValue( "metafileminimum", 500 );
				if( GlobalSize(hmem)>md->minimumMetafileSize ) {
					md->compressor->close( new GlobalMem(hmem) );
				} else {
					md->compressor->close( 0 );
					GlobalFree( hmem );
				}
				
				LeaveCriticalSection( &d->cs );
				
				// draw again for local display
				(*d->originalGaugeCallback)( d->gaugeHeader, service_id, extra_data );
				
				return;
			}				
			LeaveCriticalSection( &d->cs );
		} break;
		case MulticrewCore::ClientMode:	{
            // display
			EnterCriticalSection( &d->cs );
			SmartPtr<GlobalMem> lastMetafile = md->decompressor->lastMetafile();
			if( d->elements.size()>md->metafileElement && 
				d->elements[md->metafileElement] && 
				!lastMetafile.isNull() &&
				md->metafileCounter!=md->decompressor->counter() ) {
				//dout << "draw metafile" << std::endl;
				
				// don't show same again
				md->metafileCounter =  md->decompressor->counter();
				
				// get element to draw onto
				Element *element = d->elements[md->metafileElement];
				PELEMENT_STATIC_IMAGE staticImage = (PELEMENT_STATIC_IMAGE)element->elementHeader();
				PPIXPOINT dim = &staticImage->image_data.final->dim;
				
				// create stream
				IStream *stream;
				CreateStreamOnHGlobal( lastMetafile->handle(), FALSE, &stream );			
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
				SET_OFF_SCREEN( staticImage );
			}
			LeaveCriticalSection( &d->cs );
			return;
		} break;
		}
	} break;

	case PANEL_SERVICE_DISCONNECT:
		GdiplusShutdown(d->gdiplusToken);
		break;
		
	default:
		break;
	}
	
	// call inherited callback
	Gauge::callback( pgauge, service_id, extra_data );
}
