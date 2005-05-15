/* Microsoft Flight Simulator 2004 Module Example.
 *
 * Copyright (c) 2004, Cyril Hruscak.
 * You may freely redistribute and/or modify this code provided this copyright
 * notice remains unchanged. Usage of the code is at your own risk. I accept
 * no liability for any possible damage using this code.
 *
 * It shows how to create a module dll ("Modules" directory of the main
 * FS directory) that can be loaded by the FS2004. It also shows how to add
 * a new menu entry to the main flight simulator menu.
 */

#include "common.h"

#include <wx/wx.h>
#include <windows.h>
#include "multicrewui.h"
#include "../multicrewgauge/GAUGES.h"
#include "../multicrewcore/thread.h"


/************************** gauge/module linkage ****************************/
void FSAPI module_init(void) {
}


void FSAPI module_deinit(void) {
}


GAUGESLINKAGE Linkage = {
	0x00000000,
	module_init,
	module_deinit,
	0,
	0,
	0x0900,	// FS2004 version (use 0x0800 for FS2002)
	NULL
};

GAUGESIMPORT ImportTable;


/*************************** Win32 window management *************************/
class wxDLLApp : public wxApp {
	bool OnInit() { return true; }
};

IMPLEMENT_APP_NO_MAIN(wxDLLApp)

WNDPROC oldWndProc;
HWND hFSimWindow;
HINSTANCE gInstance;
#define	MENU_ENTRY	"Multi&crew"
LRESULT CALLBACK FSimWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/*************************** ui thread ***************************************/
class UIThread : public Thread {
public:
	UIThread() {
		startThread( 0 );

		// load multicrewgauge.dll (it mustn't be unloaded because then
		// the hooks get invalid and might produce crashes)
		dout << "Loading multicrewgauge.dll" << std::endl;
		hMulticrewGauge = LoadLibrary( "multicrew\\multicrewgauge.dll" );
	}

	virtual ~UIThread() {
		// stop ui thread
		postThreadMessage( WM_QUIT, 0, 0 );
		stopThread();
	}

	virtual unsigned threadProc( void *param ) {
		// setup wx
		wxEntry( gInstance, 0, NULL, 0, false );

		// setup ui
		dout << "Creating MulticrewUI" << std::endl;
		ui = new MulticrewUI( hFSimWindow );

		// hook FS window
		oldWndProc = (WNDPROC)SetWindowLong( hFSimWindow, GWL_WNDPROC, (LONG)FSimWindowProc );

		// thread message queue
		MSG msg;
		BOOL ret; 
		dout << "Starting MulticrewUI thread" << std::endl;
		while( (ret=GetMessage( &msg, NULL, 0, 0 ))!=0 ) {
			//dout << "MulticrewUI thread message " << msg.message << std::endl;
			switch( msg.message ) {
			case WM_COMMAND:
			{
				//dout << "message was WM_COMMAND " << LOWORD(msg.wParam) << std::endl;
				switch( LOWORD(msg.wParam) ) {
				case ID_HOST_MENUITEM: ui->host(); break;
				case ID_CONNECT_MENUITEM: ui->connect(); break;
				case ID_DISCONNECT_MENUITEM: ui->disconnect(); break;
				case ID_STATUS_MENUITEM: ui->status(); break;
				case ID_LOG: ui->log(); break;
				case ID_ASYNC: ui->async(); break;
				}
			} break;
			}
			DispatchMessage(&msg); 
		}

		// shutdown ui
		dout << "Shutting down MulticrewUI" << std::endl;
		delete ui;

		// shutdown wx
		wxGetApp().OnExit();
		wxApp::CleanUp();
		//wxEntryCleanup(); // crashes :(
		//wxUninitialize();

		return true;
	}

	MulticrewUI *ui;
	HMODULE hMulticrewGauge;
};


UIThread *ui;


/*****************************************************************************/
LRESULT CALLBACK FSimWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_NCPAINT: {
			// add a menu to the fs window
			HMENU hFSMenu;

			hFSMenu = GetMenu(hwnd);
			if( hFSMenu!=NULL ) {
				int i;
				// Look for our menu entry in the main menu.
				for( i=0; i<GetMenuItemCount(hFSMenu); i++ ) {
					char buf[128];
					GetMenuString( hFSMenu, i, buf, 128, MF_BYPOSITION );
					if( strcmp(buf, MENU_ENTRY)==0 ) {
						// It is already here, we do not need to add it again
						break;
					}
				}
				if( i<GetMenuItemCount(hFSMenu) ) {
					// It is already here, we do not need to add it again
					break;
				}
				
				/* Create new menu. NOTE: It seems that this will be
				 * reached more times, so we cannot save the handle, because
				 * in such case it could be destroyed and we will not have
				 * any access to it in the simulator.
				 */
				// add the created menu to the main menu
				AppendMenu(hFSMenu, MF_STRING | MF_POPUP, (UINT_PTR)ui->ui->newMenu(), MENU_ENTRY);		
			}
		}
		break;
		
		case WM_COMMAND:
			if( LOWORD(wParam)>=ID_FIRST && LOWORD(wParam)<=ID_LAST )
				ui->postThreadMessage( uMsg, wParam, lParam );
			break;
	}

	// Call the original window procedure to handle all other messages
	return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}


/*****************************************************************************/
BOOL WINAPI DllMain( HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
	switch( fdwReason ) {
	case DLL_PROCESS_ATTACH: {
		OutputDebugString( "Loading MulticrewUI\n" );
		
		hFSimWindow = FindWindow("FS98MAIN", NULL);
		gInstance = hInstDLL;
		ui = new UIThread();
		
		OutputDebugString( "Loaded MulticrewUI\n" );
	} break;		
	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH:	break;
	case DLL_PROCESS_DETACH: {
		OutputDebugString( "Unloading MulticrewUI\n" );

		delete ui;			

		OutputDebugString( "Unloaded MulticrewUI\n" );
	} break;
	}
	return TRUE;
}
