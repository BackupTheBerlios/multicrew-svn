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



#include <wx/wx.h>
#include <wx/settings.h>
#include <wx/dialog.h>

#include "resource.h"

class StatusDialog:public wxDialog
{
public:
 StatusDialog(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE, const wxString& name = wxT("dialogBox"));
 virtual ~StatusDialog();

	void setConnected();
	void setUnconnected();

private:
	struct Data;
	friend Data;
	Data *d;
	
	void VwXinit();
	void planeRegistered();
	void planeUnregistered();
	void logged( const char *line );

	DECLARE_EVENT_TABLE()  

	#define VwX_INIT_OBJECTS_StatusDialog
};