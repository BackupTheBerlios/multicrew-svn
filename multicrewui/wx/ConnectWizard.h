
// Don't modify comment 
#ifndef __VwX_ConnectWizard_H__
#define __VwX_ConnectWizard_H__
#include <wx/settings.h>
#include <wx/dialog.h>
#include <wx/wizard.h>
 
//[inc]add your include files here

//[inc]end your include
 
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
class ConnectWizard;
class ConnectWizardEvt:public wxEvtHandler
{
public:
 ConnectWizardEvt(ConnectWizard *parent){ptr_winPan=parent;}
protected:  
 ConnectWizard *ptr_winPan;
 DECLARE_EVENT_TABLE()     
//[evt]add your code here

//[evt]end your code
};

class ConnectWizard:public wxWizard
{
 friend class ConnectWizardEvt;
public:
 ConnectWizard(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""),const wxBitmap& bitmap = wxNullBitmap, const wxPoint& pos = wxDefaultPosition);
// Pointer control
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
 virtual ~ConnectWizard();
 virtual void DConnectWizard();
 virtual void initBefore();
 void OnPreCreate();
 void VwXinit();
 bool RunWizard();
 virtual void initAfter();
protected:
 DECLARE_EVENT_TABLE()  
//[win]add your code here

 #define VwX_INIT_OBJECTS_ConnectWizard
//[win]end your code 
};
// end ConnectWizard

#endif
