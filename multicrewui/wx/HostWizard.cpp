
// Don't modify comment 
#include "HostWizard.h"
//[inc]add your include files here


//[inc]end your include
 

HostWizard::HostWizard(wxWindow* parent,wxWindowID id,const wxString& title,const wxBitmap& bitmap,const wxPoint& pos)
  VwX_INIT_OBJECTS_HostWizard
{
 wxBitmap m_bitmap=bitmap;
 if(m_bitmap==wxNullBitmap){
     m_bitmap= wxBitmap(_T("wizard.bmp"),wxBITMAP_TYPE_BMP);
 }
 OnPreCreate();
 Create(parent,id,title,m_bitmap,pos);

 initBefore();
 VwXinit();initAfter();
}
HostWizard::~HostWizard()
{
  DHostWizard();
}
void HostWizard::VwXinit()
{
 SetTitle(wxT("Multicrew Session Wizard"));
 startPage=new wxWizardPageSimple(this);
   startPage->SetSize(0,0,20,20);
   FitToPage(startPage);
 st4=new wxStaticText(startPage,-1,wxT(""),wxPoint(0,0),wxSize(315,260),wxST_NO_AUTORESIZE);
   st4->SetLabel(wxT("You are going to host a Multicrew session. Remote computers will be able to join your Multicrew game if they own exactly the same aircraft. Make sure to use a password if you don't want that unknown people connect to your session.\r\n\r\nIf you use a network router or a firewall software you have to forward or open the used UDP port. Otherwise the data of the remote computers can not reach your Multicrew aircraft. The same is true for the remote computers and your data packets. If the owner of the remote computers don't open and/or forward the used UDP port your packets cannot reach their Multicrew aircrafts."));
 sessionPage=new wxWizardPageSimple(this);
   sessionPage->SetSize(139,12,318,260);
   FitToPage(sessionPage);
 st6=new wxStaticText(sessionPage,-1,wxT(""),wxPoint(0,0),wxSize(315,63),wxST_NO_AUTORESIZE);
   st6->SetLabel(wxT("Enter the data of your Multicrew session. Without a password every user who owns exactly the same aircraft can connect to your session. Remote users will see the session name to identify your aircraft."));
 portEdit=new wxTextCtrl(sessionPage,-1,wxT(""),wxPoint(100,166),wxSize(55,21));
   portEdit->SetLabel(wxT("1234"));
 st8=new wxStaticText(sessionPage,-1,wxT(""),wxPoint(0,170),wxSize(45,13),wxST_NO_AUTORESIZE);
   st8->SetLabel(wxT("Port:"));
 providersCombo=new wxComboBox(sessionPage,-1,wxT(""),wxPoint(100,135),wxSize(215,21),0,NULL);
   providersCombo->Enable(false);
 st10=new wxStaticText(sessionPage,-1,wxT(""),wxPoint(0,136),wxSize(90,18),wxST_NO_AUTORESIZE);
   st10->Enable(false);
   st10->SetLabel(wxT("Service provider:"));
 st11=new wxStaticText(sessionPage,-1,wxT(""),wxPoint(0,110),wxSize(85,13),wxST_NO_AUTORESIZE);
   st11->SetLabel(wxT("Session name:"));
 sessionNameEdit=new wxTextCtrl(sessionPage,-1,wxT(""),wxPoint(100,105),wxSize(215,21));
   sessionNameEdit->SetLabel(wxT("Multicrew Session"));
 st13=new wxStaticText(sessionPage,-1,wxT(""),wxPoint(0,230),wxSize(85,13),wxST_NO_AUTORESIZE);
   st13->SetLabel(wxT("Password:"));
 passwordEdit=new wxTextCtrl(sessionPage,-1,wxT(""),wxPoint(125,225),wxSize(190,21));
   passwordEdit->Enable(false);
 passwordEnabledCheck=new wxCheckBox(sessionPage,-1,wxT(""),wxPoint(105,230),wxSize(18,13));

 startPage->SetPrev(NULL);startPage->SetNext(sessionPage);
 sessionPage->SetPrev(startPage);sessionPage->SetNext(NULL);
 Refresh();
}
bool HostWizard::RunWizard()
{
 wxWizard::RunWizard(startPage);
}
 
BEGIN_EVENT_TABLE(HostWizardEvt,wxEvtHandler)
//[evtEvt]add your code here


//[evtEvt]end your code
END_EVENT_TABLE()
 
BEGIN_EVENT_TABLE( HostWizard,wxWizard)
//[evtwin]add your code here


//[evtwin]end your code
END_EVENT_TABLE()

//[evtFunc]add your code here

void HostWizard::initBefore(){
 //add your code here

}

void HostWizard::initAfter(){
 //add your code here

}

void HostWizard::OnPreCreate(){
 //add your code here

}

void HostWizard::DHostWizard(){
 //add your code here

}

//[evtFunc]end your code
