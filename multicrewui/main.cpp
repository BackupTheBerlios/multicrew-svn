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
#include "module.h"

/*
 * FS linking
 */
UI_DLLEXPORT MODULE_IMPORT ImportTable = {
	{0x00000000, NULL},
	{0x00000000, NULL}
};

void FSAPI module_init(void) {}
void FSAPI module_deinit(void) {}

extern "C" UI_DLLEXPORT MODULE_LINKAGE Linkage;
UI_DLLEXPORT MODULE_LINKAGE Linkage = {
	0x00000000,
	module_init,
	module_deinit,
	0,
	0,
	0x0900,	// FS2004 version (use 0x0800 for FS2002)
	NULL
};

/*
 * Win32 window management
 */

class wxDLLApp : public wxApp {
	bool OnInit() { return true; }
};

IMPLEMENT_APP_NO_MAIN(wxDLLApp)

MulticrewUI *ui;
WNDPROC oldWndProc;
HWND hFSimWindow;

#define	MENU_ENTRY	"Multi&crew"

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
				AppendMenu(hFSMenu, MF_STRING | MF_POPUP, (UINT_PTR)ui->newMenu(), MENU_ENTRY);		
			}
		}
		break;
		
		case WM_COMMAND:
			switch( LOWORD(wParam) ) {
				case ID_HOST_MENUITEM:
					ui->host();
					return 0;
				case ID_CONNECT_MENUITEM:
					ui->connect();
					return 0;
				case ID_DISCONNECT_MENUITEM:
					ui->disconnect();
					return 0;
				case ID_STATUS_MENUITEM:
					ui->status();
					return 0;
				case ID_LOG:
					ui->log();
					return 0;
				case ID_ASYNC:
					ui->async();
					return 0;
			}
			break;
	}
	// Call the original window procedure to handle all other messages
	return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

/**
 * Entry point of the DLL.
 */
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH: {
			// setup wx
			int argc = 0;
			char **argv = new char*;
			argv[0] = NULL;
			
			/*wxInitialize();
			wxSetInstance( hInstDLL );			
			wxEntryStart( argc, argv );*/
			wxEntry( hInstDLL, 0, NULL, 0, false );

			// find FS windows
			hFSimWindow = FindWindow("FS98MAIN", NULL);

			// setup ui
			ui = new MulticrewUI( hFSimWindow );

			// hook FS window
			oldWndProc = (WNDPROC)SetWindowLong(hFSimWindow, GWL_WNDPROC, (LONG)FSimWindowProc);					 
		} break;		
		case DLL_PROCESS_DETACH:
			delete ui;

			// shutdown wx
			wxGetApp().OnExit();
			wxApp::CleanUp();
			//wxEntryCleanup(); // crashes :(
			//wxUninitialize();
			break;
	}
	return TRUE;
}
