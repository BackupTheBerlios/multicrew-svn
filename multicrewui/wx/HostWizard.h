
// Don't modify comment 
#ifndef __VwX_HostWizard_H__
#define __VwX_HostWizard_H__
#include <wx/settings.h>
#include <wx/dialog.h>
#include <wx/wizard.h>
 
//[inc]add your include files here



//[inc]end your include
 
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
class HostWizard;
class HostWizardEvt:public wxEvtHandler
{
public:
 HostWizardEvt(HostWizard *parent){ptr_winPan=parent;}
protected:  
 HostWizard *ptr_winPan;
 DECLARE_EVENT_TABLE()     
//[evt]add your code here



//[evt]end your code
};

class HostWizard:public wxWizard
{
 friend class HostWizardEvt;
public:
 HostWizard(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""),const wxBitmap& bitmap = wxNullBitmap, const wxPoint& pos = wxDefaultPosition);
// Pointer control
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
 virtual ~HostWizard();
 virtual void DHostWizard();
 virtual void initBefore();
 void OnPreCreate();
 void VwXinit();
 bool RunWizard();
 virtual void initAfter();
protected:
 DECLARE_EVENT_TABLE()  
//[win]add your code here



 #define VwX_INIT_OBJECTS_HostWizard
//[win]end your code 
};
// end HostWizard

#endif
