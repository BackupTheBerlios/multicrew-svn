
// Don't modify comment 
#include "ConnectWizard.h"
//[inc]add your include files here


//[inc]end your include
 

ConnectWizard::ConnectWizard(wxWindow* parent,wxWindowID id,const wxString& title,const wxBitmap& bitmap,const wxPoint& pos)
  VwX_INIT_OBJECTS_ConnectWizard
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
ConnectWizard::~ConnectWizard()
{
  DConnectWizard();
}
void ConnectWizard::VwXinit()
{
 SetTitle(wxT("Multicrew Connection Wizard"));
 startPage=new wxWizardPageSimple(this);
   startPage->SetSize(139,12,318,250);
   FitToPage(startPage);
 hostPage=new wxWizardPageSimple(this);
   hostPage->SetSize(134,12,328,250);
   FitToPage(hostPage);
 st33=new wxStaticText(startPage,-1,wxT(""),wxPoint(0,0),wxSize(320,255),wxST_NO_AUTORESIZE);
   st33->SetLabel(wxT("You are going to connect to a Multicrew session. The remote computer must host a session with exactly the same aircraft. \r\n\r\nIf you use a network router or a firewall software you have to forward or open the used UDP port. Otherwise the data of the host can not reach your Multicrew aircraft. The same is true for the remote computer and your data packets. If the owner of the remote computer doesn't open and/or forward the used UDP port your packets cannot reach his Multicrew session."));
 st34=new wxStaticText(hostPage,-1,wxT(""),wxPoint(5,0),wxSize(325,68),wxST_NO_AUTORESIZE);
   st34->SetLabel(wxT("Search for games in the local network or enter a specific address. This can be a raw IP address (e.g. 192.168.0.7) or a fully qualified hostname with domain (e.g. multicrew.homeip.net). The port number normally does not to be changed. It must match though with the port that the host uses."));
 hostName=new wxTextCtrl(hostPage,-1,wxT(""),wxPoint(150,136),wxSize(160,21));
   hostName->Enable(false);
   hostName->SetLabel(wxT("Text"));
 port=new wxTextCtrl(hostPage,-1,wxT(""),wxPoint(150,160),wxSize(45,21));
   port->Enable(false);
   port->SetLabel(wxT("Text"));
 hostNameStatic=new wxStaticText(hostPage,-1,wxT(""),wxPoint(10,140),wxSize(135,13),wxST_NO_AUTORESIZE);
   hostNameStatic->Enable(false);
   hostNameStatic->SetLabel(wxT("Hostname or IP address:"));
 st38=new wxStaticText(hostPage,-1,wxT(""),wxPoint(10,164),wxSize(45,13),wxST_NO_AUTORESIZE);
   st38->Enable(false);
   st38->SetLabel(wxT("Port:"));
 sessionlistPage=new wxWizardPageSimple(this);
   sessionlistPage->SetSize(129,12,328,250);
   FitToPage(sessionlistPage);
 sessions=new wxListBox(sessionlistPage,-1,wxPoint(0,15),wxSize(320,93),0,NULL);
 st41=new wxStaticText(sessionlistPage,-1,wxT(""),wxPoint(0,0),wxSize(115,13),wxST_NO_AUTORESIZE);
   st41->SetLabel(wxT("Found sessions:"));
 lno46=new wxStaticLine(sessionlistPage,-1,wxPoint(0,120),wxSize(315,2));
 broadcastRadio=new wxRadioButton(hostPage,-1,wxT(""),wxPoint(10,100),wxSize(298,13));
   broadcastRadio->SetValue(true);
   broadcastRadio->SetTitle(wxT("Search for games in the local network"));
 directRadio=new wxRadioButton(hostPage,-1,wxT(""),wxPoint(10,120),wxSize(298,13));
   directRadio->SetTitle(wxT("Connect with specific host"));

 startPage->SetPrev(NULL);startPage->SetNext(hostPage);
 hostPage->SetPrev(startPage);hostPage->SetNext(sessionlistPage);
 sessionlistPage->SetPrev(hostPage);sessionlistPage->SetNext(NULL);
 Refresh();
}
bool ConnectWizard::RunWizard()
{
 wxWizard::RunWizard(startPage);
}
 
BEGIN_EVENT_TABLE(ConnectWizardEvt,wxEvtHandler)
//[evtEvt]add your code here


//[evtEvt]end your code
END_EVENT_TABLE()
 
BEGIN_EVENT_TABLE( ConnectWizard,wxWizard)
//[evtwin]add your code here


//[evtwin]end your code
END_EVENT_TABLE()

//[evtFunc]add your code here

void ConnectWizard::initBefore(){
 //add your code here

}

void ConnectWizard::initAfter(){
 //add your code here

}

void ConnectWizard::OnPreCreate(){
 //add your code here

}

void ConnectWizard::DConnectWizard(){
 //add your code here

}

bool ConnectWizard::TransferDataFromWindow() {
    wxIPV4address addr;
    if( addr.Hostname( hostName->GetValue() )==false ) {
        wxMessageBox(_T("Please enter a valid internet (IP v4) address for the hostname."), _T("Invalid hostname"),
                    wxICON_ERROR | wxOK, this);
        return false;
    }
                
    if( addr.Service( port->GetValue() )==false ) {
        wxMessageBox(_T("Please enter a valid port number (between 1 and 65535)."), _T("Invalid port"),
                    wxICON_ERROR | wxOK, this);
        return false;
    }

    return true;
}
//[evtFunc]end your code
