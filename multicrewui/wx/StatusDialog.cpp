
// Don't modify comment 
#include "StatusDialog.h"
//[inc]add your include files here


//[inc]end your include
 

StatusDialog::StatusDialog(wxWindow* parent,wxWindowID id,const wxString& title,const wxPoint& pos,const wxSize& size,long style,const wxString& name)
  VwX_INIT_OBJECTS_StatusDialog
{
 OnPreCreate();
 Create(parent,id,title,pos,size,style,name);

 if((pos==wxDefaultPosition)&&(size==wxDefaultSize)){
     SetSize(0,0,390,315);
 }

 if((pos!=wxDefaultPosition)&&(size==wxDefaultSize)){
     SetSize(390,315);
 }
 initBefore();
 VwXinit();initAfter();
}
StatusDialog::~StatusDialog()
{
  DStatusDialog();
}
void StatusDialog::VwXinit()
{
 SetTitle(wxT("Multicrew Status"));
 logs=new wxTextCtrl(this,-1,wxT(""),wxPoint(10,85),wxSize(365,193),wxTE_MULTILINE);
   logs->SetLabel(wxT("Text"));
 st4=new wxStaticText(this,-1,wxT(""),wxPoint(10,65),wxSize(365,13),wxST_NO_AUTORESIZE);
   st4->SetLabel(wxT("Log output:"));
 st5=new wxStaticText(this,-1,wxT(""),wxPoint(40,15),wxSize(340,13),wxST_NO_AUTORESIZE);
   st5->SetLabel(wxT("Multicrew plane connected"));
 st9=new wxStaticText(this,-1,wxT(""),wxPoint(40,40),wxSize(340,13),wxST_NO_AUTORESIZE);
   st9->SetLabel(wxT("Connected to host"));
 planeConnected=new wxCheckBox(this,-1,wxT(""),wxPoint(10,15),wxSize(13,13));
   planeConnected->Enable(false);
 hostConnected=new wxCheckBox(this,-1,wxT(""),wxPoint(10,40),wxSize(13,13));
   hostConnected->Enable(false);
 Refresh();
}
 
BEGIN_EVENT_TABLE(StatusDialogEvt,wxEvtHandler)
//[evtEvt]add your code here


//[evtEvt]end your code
END_EVENT_TABLE()
 
BEGIN_EVENT_TABLE( StatusDialog,wxDialog)
//[evtwin]add your code here


//[evtwin]end your code
END_EVENT_TABLE()

//[evtFunc]add your code here

void StatusDialog::initBefore(){
 //add your code here

}

void StatusDialog::initAfter(){
 //add your code here

}

void StatusDialog::OnPreCreate(){
 //add your code here

}

void StatusDialog::DStatusDialog(){
 //add your code here

}

//[evtFunc]end your code
