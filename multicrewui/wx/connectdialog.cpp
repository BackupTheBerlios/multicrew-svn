
// Don't modify comment 
#include "connectdialog.h"
//[inc]add your include files here


//[inc]end your include
 

ConnectDialog::ConnectDialog(wxWindow* parent,wxWindowID id,const wxString& title,const wxPoint& pos,const wxSize& size,long style,const wxString& name)
  VwX_INIT_OBJECTS_ConnectDialog
{
 OnPreCreate();
 Create(parent,id,title,pos,size,style,name);

 if((pos==wxDefaultPosition)&&(size==wxDefaultSize)){
     SetSize(-1,-1,260,135);
 }

 if((pos!=wxDefaultPosition)&&(size==wxDefaultSize)){
     SetSize(260,135);
 }
 initBefore();
 VwXinit();initAfter();
}
ConnectDialog::~ConnectDialog()
{
  DConnectDialog();
}
void ConnectDialog::VwXinit()
{
 SetTitle(wxT("Multicrew - Connect to host"));
 connectButton=new wxButton(this,ID_OK,wxT(""),wxPoint(35,65),wxSize(85,30));
   connectButton->SetLabel(wxT("C&onnect"));
 cancelButton=new wxButton(this,ID_CANCEL,wxT(""),wxPoint(130,65),wxSize(85,30));
   cancelButton->SetLabel(wxT("&Cancel"));
 hostStatic=new wxStaticText(this,-1,wxT(""),wxPoint(15,15),wxSize(70,13),wxST_NO_AUTORESIZE);
   hostStatic->SetLabel(wxT("Hostname:"));
 portStatic=new wxStaticText(this,-1,wxT(""),wxPoint(15,39),wxSize(45,13),wxST_NO_AUTORESIZE);
   portStatic->SetLabel(wxT("Port:"));
 hostEdit=new wxTextCtrl(this,-1,wxT(""),wxPoint(80,11),wxSize(160,21));
   hostEdit->SetLabel(wxT("Text"));
 portEdit=new wxTextCtrl(this,-1,wxT(""),wxPoint(80,35),wxSize(45,21));
 Refresh();
}
 
BEGIN_EVENT_TABLE(ConnectDialogEvt,wxEvtHandler)
//[evtEvt]add your code here
   

//[evtEvt]end your code
END_EVENT_TABLE()
 
BEGIN_EVENT_TABLE( ConnectDialog,wxDialog)
//[evtwin]add your code here
    EVT_BUTTON(ID_OK, ConnectDialog::OnOK)
    EVT_BUTTON(ID_CANCEL, ConnectDialog::OnCancel)
//[evtwin]end your code
END_EVENT_TABLE()

//[evtFunc]add your code here

void ConnectDialog::initBefore(){
 //add your code here

}

void ConnectDialog::initAfter(){
 //add your code here

}

void ConnectDialog::OnPreCreate(){
 //add your code here

}

void ConnectDialog::DConnectDialog(){
 //add your code here

}

//[evtFunc]end your code
