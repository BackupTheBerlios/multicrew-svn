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
#include <wx/wizard.h>
#include <wx/statline.h>
#include <wx/socket.h>

#include "resource.h"
#include "../multicrewcore/network.h"
#include "../multicrewcore/shared.h"


class ConnectWizard : public wxWizard { 
public:
 ConnectWizard(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""),const wxBitmap& bitmap = wxNullBitmap, const wxPoint& pos = wxDefaultPosition);
 virtual ~ConnectWizard();

 SmartPtr<Connection> RunWizard();

private:
 wxWizardPageSimple *startPage;
 wxWizardPageSimple *hostPage;
 wxStaticText *st33;
 wxStaticText *st34;
 wxTextCtrl *hostName;
 wxTextCtrl *port;
 wxStaticText *hostNameStatic;
 wxStaticText *st38;
 wxWizardPageSimple *sessionlistPage;
 wxListBox *sessions;
 wxStaticText *st41;
 wxStaticLine *lno46;
 wxRadioButton *broadcastRadio;
 wxRadioButton *directRadio;
 void VwXinit(); 
 
 void OnNextPage( wxWizardEvent& event );
 void OnPageChanged( wxWizardEvent& event );
 void OnFinished( wxWizardEvent& event );
 void OnHostFound( SmartPtr<ClientConnectionSetup::FoundHost> foundHost );
 void OnRadioButton( wxCommandEvent& event );
 DECLARE_EVENT_TABLE()  

#define VwX_INIT_OBJECTS_ConnectWizard

 struct Data;
 friend Data;
 Data *d;
};

