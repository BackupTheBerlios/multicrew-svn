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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/streams.h"
#include "../multicrewcore/signals.h"
#include "../multicrewcore/network.h"
#include "ConnectWizard.h"
#include "HostWizard.h"
#include "StatusDialog.h"
#include "multicrewui.h"

struct MulticrewUI::Data {
	Data( MulticrewUI *ui ) 
		: core( MulticrewCore::multicrewCore() ),
		  connection( __FILE__, __LINE__ ),
		  planeRegisteredSlot( &core->planeLoaded, ui, MulticrewUI::planeRegistered ),
		  planeUnregisteredSlot( &core->planeUnloaded, ui, MulticrewUI::planeUnregistered ),
		  disconnectedSlot( 0, ui, MulticrewUI::disconnected ),
		  statusDlg( NULL ),
		  loggedSlot( &core->logged, ui, MulticrewUI::logged ),
		  asyncSlot( &core->initAsyncCallback, ui, MulticrewUI::asyncSlot ) {		
	}

	SmartPtr<MulticrewCore> core;
	Slot<MulticrewUI> planeRegisteredSlot;
	Slot<MulticrewUI> planeUnregisteredSlot;

	StatusDialog *statusDlg;
	
	SmartPtr<Connection> connection; 
	Slot<MulticrewUI> disconnectedSlot;

	Slot1<MulticrewUI, const char*> loggedSlot;
	CRITICAL_SECTION loggedCritSect;
	std::list<std::string> loggedLines;

	Slot<MulticrewUI> asyncSlot;

	HMENU menu;
	HWND hwnd;
	wxWindow *mainWindow;
};


MulticrewUI::MulticrewUI( HWND hwnd ) {
	d = new Data( this );
	d->menu = 0;
	d->hwnd = hwnd;
	d->mainWindow= new wxWindow();
	d->mainWindow->SetHWND( (WXHWND)hwnd );
	d->statusDlg = new StatusDialog( d->mainWindow );
	d->mainWindow->Enable( false );

	InitializeCriticalSection( &d->loggedCritSect );
}


MulticrewUI::~MulticrewUI() {
	dout << "> ~MulticrewUI()" << std::endl;

	// disconnect network connections
	if( !d->connection.isNull() ) {
		d->disconnectedSlot.disconnect();
		d->core->unprepare();
		d->connection->disconnect();
		d->connection = 0;
	}

	// destroy status dialog
	d->statusDlg->Destroy();
	
	// disconnect from main window
	d->mainWindow->SetHWND( 0 );
	d->mainWindow->Destroy();
	DeleteCriticalSection( &d->loggedCritSect );
	delete d;

#ifdef SHARED_DEBUG
	SmartPtrBase::dumpPointers();
#endif
	dout << "< ~MulticrewUI()" << std::endl;
}


void MulticrewUI::logged( const char *line ) {
	EnterCriticalSection( &d->loggedCritSect );
	d->loggedLines.push_back( line );
	LeaveCriticalSection( &d->loggedCritSect );
	PostMessage( d->hwnd, WM_COMMAND, ID_LOG, 0 );
}


void MulticrewUI::log() {
	EnterCriticalSection( &d->loggedCritSect );
	std::list<std::string>::iterator it = d->loggedLines.begin();
	while( it!=d->loggedLines.end() ) {
		d->statusDlg->log( *it );
		it++;
	}
	d->loggedLines.clear();
	LeaveCriticalSection( &d->loggedCritSect );
}


void MulticrewUI::host() {
	HostWizard dlg( d->mainWindow );
	bool ret = dlg.RunWizard();
	if( ret ) {
		HostConnectionSetup setup;	   
		char password[256];
		wcstombs( password, dlg.password().c_str(), 256 );
		char name[256];
		wcstombs( name, dlg.sessionName().c_str(), 256 );
		SmartPtr<Connection> con( setup.host( dlg.port(), 
											  name,
											  dlg.passwordEnabled(), 
											  password ) );
		if( con.isNull() )
			derr << "Session creation failed. Take a look at the logs to find out why." << std::endl;
		else {
			d->connection = con;
			d->disconnectedSlot.connect( &d->connection->disconnected );
			d->core->prepare( d->connection );
			bool ok = d->connection->start();
			if( !ok ) {
				d->core->unprepare();
				d->connection->disconnect();
				d->connection = 0;
				//derr << "Session start failed. Take a look at the logs to find out why." 
				//	 << std::endl;
			} else
				d->statusDlg->setConnected();
		}
	}
}


void MulticrewUI::status() {
	if( d->statusDlg->IsShown() )
		d->statusDlg->Hide();
	else
		d->statusDlg->Show();
}


void MulticrewUI::connect() {
	ConnectWizard dlg( d->mainWindow );
	dout << "run" << std::endl;
	SmartPtr<Connection> ret = dlg.RunWizard();
	if( !ret.isNull() ) {
		d->connection = ret;
		d->disconnectedSlot.connect( &d->connection->disconnected );
		d->core->prepare( d->connection );
		d->connection->start();
		d->statusDlg->setConnected();
	}
}


void MulticrewUI::disconnect() {
	if( !d->connection.isNull() ) {
		int ret = MessageBox(d->hwnd, "Really disconnect?", "Multicrew", 
			MB_OKCANCEL | MB_ICONQUESTION);
		if( ret==IDOK ) {
			d->core->unprepare();
			d->connection->disconnect();
		}
	}
}


void MulticrewUI::disconnected() {
	d->connection = 0;
	updateMenu();
	d->statusDlg->setUnconnected();
	MessageBox( d->hwnd, "Session terminated.", "Multicrew", MB_OK | MB_ICONINFORMATION );
}


void MulticrewUI::planeRegistered() {	
	updateMenu();
}


void MulticrewUI::planeUnregistered() {
	d->connection = 0;
	updateMenu();
}


HMENU MulticrewUI::newMenu() {
	// delete old menu
	if( d->menu ) DestroyMenu( d->menu );
	d->menu = CreateMenu();
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_HOST_MENUITEM, "&Host");
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_CONNECT_MENUITEM, "&Connect");
	AppendMenu(d->menu, MF_SEPARATOR, 0, 0);
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_DISCONNECT_MENUITEM, "&Disconnect");
	AppendMenu(d->menu, MF_SEPARATOR, 0, 0);
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_STATUS_MENUITEM, "&Status");
	updateMenu();

	return d->menu;
}


void MulticrewUI::updateMenu() {
	if( d->menu!=0 ) {
		bool loaded = d->core->isPlaneLoaded();
		bool hostMode = d->core->isHostMode();
		bool connected = !d->connection.isNull();

		MENUITEMINFO change;
		ZeroMemory( &change, sizeof(MENUITEMINFO) );
		change.cbSize = sizeof(MENUITEMINFO);
		change.fMask = MIIM_STATE;
		
		change.fState = (!connected && loaded && hostMode)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_HOST_MENUITEM, FALSE, &change );

		change.fState = (!connected && loaded && !hostMode)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_CONNECT_MENUITEM, FALSE, &change );

		change.fState = (connected)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_DISCONNECT_MENUITEM, FALSE, &change );
	}
}


void MulticrewUI::async() {
	d->core->callbackAsync();
}


void MulticrewUI::asyncSlot() {
	PostMessage( d->hwnd, WM_COMMAND, ID_ASYNC, 0 );
}
