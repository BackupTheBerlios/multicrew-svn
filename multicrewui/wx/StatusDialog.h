
// Don't modify comment 
#ifndef __VwX_StatusDialog_H__
#define __VwX_StatusDialog_H__
#include <wx/settings.h>
#include <wx/dialog.h>
 
//[inc]add your include files here



//[inc]end your include
 
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
class StatusDialog;
class StatusDialogEvt:public wxEvtHandler
{
public:
 StatusDialogEvt(StatusDialog *parent){ptr_winPan=parent;}
protected:  
 StatusDialog *ptr_winPan;
 DECLARE_EVENT_TABLE()     
//[evt]add your code here



//[evt]end your code
};

class StatusDialog:public wxDialog
{
 friend class StatusDialogEvt;
public:
 StatusDialog(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE, const wxString& name = wxT("dialogBox"));
// Pointer control
 wxTextCtrl *logs;
 wxStaticText *st4;
 wxStaticText *st5;
 wxStaticText *st9;
 wxCheckBox *planeConnected;
 wxCheckBox *hostConnected;
 virtual ~StatusDialog();
 virtual void DStatusDialog();
 virtual void initBefore();
 void OnPreCreate();
 void VwXinit();
 virtual void initAfter();
protected:
 DECLARE_EVENT_TABLE()  
//[win]add your code here



 #define VwX_INIT_OBJECTS_StatusDialog
//[win]end your code 
};
// end StatusDialog

#endif
