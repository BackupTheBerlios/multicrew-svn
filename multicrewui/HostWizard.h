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
#include <wx/wizard.h> 
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>

#include <string>
#include "resource.h"

class HostWizard:public wxWizard
{
public:
 HostWizard(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""),const wxBitmap& bitmap = wxNullBitmap, const wxPoint& pos = wxDefaultPosition);
 virtual ~HostWizard();
 
 bool RunWizard();
 int port();
 std::wstring sessionName();
 bool passwordEnabled();
 std::wstring password();

private:
 wxWizardPageSimple *startPage;
 wxStaticText *st4;
 wxWizardPageSimple *sessionPage;
 wxStaticText *st6;
 wxTextCtrl *portEdit;
 wxStaticText *st8;
 wxComboBox *providersCombo;
 wxStaticText *st10;
 wxStaticText *st11;
 wxTextCtrl *sessionNameEdit;
 wxStaticText *st13;
 wxTextCtrl *passwordEdit;
 wxCheckBox *passwordEnabledCheck;
 void VwXinit();
 
protected:
 DECLARE_EVENT_TABLE()  

 void OnNextPage( wxWizardEvent& event );
 virtual bool TransferDataFromWindow();

 #define VwX_INIT_OBJECTS_HostWizard
};
