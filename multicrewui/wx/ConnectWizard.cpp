
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
 <<
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
