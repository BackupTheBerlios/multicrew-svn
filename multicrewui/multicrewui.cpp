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
#include "../multicrewcore/debug.h"
#include "../multicrewcore/error.h"
#include "../multicrewcore/signals.h"
#include "../multicrewcore/network.h"
#include "ConnectWizard.h"
#include "HostWizard.h"
#include "StatusDialog.h"
#include "multicrewui.h"

struct MulticrewUI::Data {
	Data( MulticrewUI *ui ) 
		: core( MulticrewCore::multicrewCore() ),
		  planeRegisteredSlot( &core->planeLoaded, ui, MulticrewUI::planeRegistered ),
		  planeUnregisteredSlot( &core->planeUnloaded, ui, MulticrewUI::planeUnregistered ),
		  hostDisconnectedSlot( 0, ui, MulticrewUI::hostDisconnected ),
		  clientDisconnectedSlot( 0, ui, MulticrewUI::clientDisconnected ),
		  statusDlg( NULL ) {		
	}

	SmartPtr<MulticrewCore> core;
	Slot<MulticrewUI> planeRegisteredSlot;
	Slot<MulticrewUI> planeUnregisteredSlot;

	StatusDialog *statusDlg;
	
	SmartPtr<HostConnection> hostConnection; 
	SmartPtr<ClientConnection> clientConnection; 
	Slot<MulticrewUI> hostDisconnectedSlot;
	Slot<MulticrewUI> clientDisconnectedSlot;

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
	//d->mainWindow.Enable( false );
}


MulticrewUI::~MulticrewUI() {
	dout << "~MulticrewUI()" << std::endl;

	// disconnect network connections
	if( !d->hostConnection.isNull() ) {
		d->hostConnection->disconnect();
		d->hostConnection = 0;
	}
	if( !d->clientConnection.isNull() ) {
		d->clientConnection->disconnect();
		d->clientConnection = 0;
	}

	// destroy status dialog
	d->statusDlg->Destroy();
	
	// disconnect from main window
	d->mainWindow->SetHWND( 0 );
	d->mainWindow->Destroy();
	delete d;
}

void MulticrewUI::host() {
	HostWizard dlg( d->mainWindow );
	bool ret = dlg.RunWizard();
	if( ret ) {
		HostConnectionSetup setup;
		SmartPtr<HostConnection> con( setup.host( dlg.port(), dlg.sessionName(),
			dlg.passwordEnabled(), dlg.password() ) );
		if( con.isNull() )
			derr << "Session creation failed. Take a look at the logs to find out why." << std::endl;
		else {
			d->hostConnection = con;
			d->hostDisconnectedSlot.connect( &d->hostConnection->disconnected );
			d->core->prepare( SmartPtr<Connection>(&*d->hostConnection) );
			d->hostConnection->start();
			d->statusDlg->setConnected();
		}
	}
}


void MulticrewUI::terminate() {
	if( !d->hostConnection.isNull() ) {
		int ret = MessageBox(d->hwnd, "Really terminate the running session?", "Multicrew", MB_OKCANCEL | MB_ICONQUESTION);
		if( ret==IDOK ) {
			d->core->unprepare();
			d->hostConnection->disconnect();
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
	SmartPtr<ClientConnection> ret = dlg.RunWizard();
	if( !ret.isNull() ) {
		d->clientConnection = ret;
		d->clientDisconnectedSlot.connect( &d->clientConnection->disconnected );
		d->core->prepare( SmartPtr<Connection>(&*d->clientConnection) );
		d->clientConnection->start();
		d->statusDlg->setConnected();
	}
}


void MulticrewUI::disconnect() {
	if( !d->clientConnection.isNull() ) {
		int ret = MessageBox(d->hwnd, "Really disconnect?", "Multicrew", MB_OKCANCEL | MB_ICONQUESTION);
		if( ret==IDOK ) {
			d->core->unprepare();
			d->clientConnection->disconnect();
		}
	}
}


void MulticrewUI::hostDisconnected() {
	d->hostConnection = 0;
	updateMenu();
	d->statusDlg->setUnconnected();
	MessageBox( d->hwnd, "Session terminated.", "Multicrew", MB_OK | MB_ICONINFORMATION );
}


void MulticrewUI::clientDisconnected() {
	d->clientConnection = 0;
	updateMenu();
	d->statusDlg->setUnconnected();
	MessageBox( d->hwnd, "Connection terminated.", "Multicrew", MB_OK | MB_ICONINFORMATION );
}


void MulticrewUI::planeRegistered() {	
	updateMenu();
}


void MulticrewUI::planeUnregistered() {
	d->clientConnection = 0;
	d->hostConnection = 0;
	updateMenu();
}


HMENU MulticrewUI::newMenu() {
	// delete old menu
	if( d->menu ) DestroyMenu( d->menu );
	d->menu = CreateMenu();
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_HOST_MENUITEM, "&Host");
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_TERMINATE_MENUITEM, "&Terminate session");	
	AppendMenu(d->menu, MF_SEPARATOR, 0, 0);
	AppendMenu(d->menu, MF_STRING | MF_ENABLED, ID_CONNECT_MENUITEM, "&Connect");
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
		bool host = !d->hostConnection.isNull();
		bool client = !d->clientConnection.isNull();

		MENUITEMINFO change;
		ZeroMemory( &change, sizeof(MENUITEMINFO) );
		change.cbSize = sizeof(MENUITEMINFO);
		change.fMask = MIIM_STATE;
		
		change.fState = (!host && loaded && hostMode)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_HOST_MENUITEM, FALSE, &change );

		change.fState = (host)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_TERMINATE_MENUITEM, FALSE, &change );

		change.fState = (!client && loaded && !hostMode)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_CONNECT_MENUITEM, FALSE, &change );

		change.fState = (client)?MFS_ENABLED:MFS_GRAYED;
		SetMenuItemInfo( d->menu, ID_DISCONNECT_MENUITEM, FALSE, &change );
	}
}
