
// Don't modify comment 
#ifndef __VwX_connectdialog_H__
#define __VwX_connectdialog_H__
#include <wx/settings.h>
#include <wx/dialog.h>
 
//[inc]add your include files here



//[inc]end your include
 
#include <wx/sizer.h>
#include <wx/button.h>
#define ID_OK 10041
#define ID_CANCEL 10042
#include <wx/stattext.h>
#include <wx/textctrl.h>
class ConnectDialog;
class ConnectDialogEvt:public wxEvtHandler
{
public:
 ConnectDialogEvt(ConnectDialog *parent){ptr_winPan=parent;}
protected:  
 ConnectDialog *ptr_winPan;
 DECLARE_EVENT_TABLE()     
//[evt]add your code here



//[evt]end your code
};

class ConnectDialog:public wxDialog
{
 friend class ConnectDialogEvt;
public:
 ConnectDialog(wxWindow* parent, wxWindowID id = -1, const wxString& title = wxT(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE, const wxString& name = wxT("dialogBox"));
// Pointer control
 wxButton *connectButton;
 wxButton *cancelButton;
 wxStaticText *hostStatic;
 wxStaticText *portStatic;
 wxTextCtrl *hostEdit;
 wxTextCtrl *portEdit;
 virtual ~ConnectDialog();
 virtual void DConnectDialog();
 virtual void initBefore();
 void OnPreCreate();
 void VwXinit();
 virtual void initAfter();
protected:
 DECLARE_EVENT_TABLE()  
//[win]add your code here



 #define VwX_INIT_OBJECTS_ConnectDialog
//[win]end your code 
};
// end ConnectDialog

#endif
