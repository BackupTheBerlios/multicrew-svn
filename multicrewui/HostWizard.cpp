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

#include "common.h"

#include "HostWizard.h"

#include "../multicrewcore/network.h"
#include "../multicrewcore/config.h"

HostWizard::HostWizard(wxWindow* parent,wxWindowID id,const wxString& title,const wxBitmap& bitmap,const wxPoint& pos)
  VwX_INIT_OBJECTS_HostWizard
{
 wxBitmap m_bitmap=bitmap;
 if( m_bitmap==wxNullBitmap ) {
     m_bitmap= wxBitmap( "BMP_HOSTWIZARD", wxBITMAP_TYPE_BMP_RESOURCE );
 }
 Create(parent,id,title,m_bitmap,pos);
 VwXinit();
 portEdit->SetValue( Config::config()->stringValue( "", "DefaultPort", MULTICREW_PORT ).c_str() );
 Refresh();
}


HostWizard::~HostWizard() {
/* 	delete st4;
	delete st6;
 	delete portEdit;
	delete st8;
	delete providersCombo;
	delete st10;
	delete st11;
	delete sessionNameEdit;
	delete st13;
	delete passwordEdit;
	delete passwordEnabledCheck;
	delete startPage;
	delete sessionPage;*/
}


void HostWizard::VwXinit() {
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
 //Refresh();
}

bool HostWizard::RunWizard() {
	return wxWizard::RunWizard(startPage);
}


int HostWizard::port() {
	long ret;
	if( !portEdit->GetValue().ToLong( &ret ) ) return -1;
	if( ret<=0 || ret>65535 ) return -1;
		
	return ret;
}


std::wstring HostWizard::sessionName() {
	return sessionNameEdit->GetValue().wc_str( wxConvLibc );
}


bool HostWizard::passwordEnabled() {
	return passwordEnabledCheck->GetValue();
}


std::wstring HostWizard::password() {
	return std::wstring( passwordEdit->GetValue().wc_str( wxConvLibc ) );
}


BEGIN_EVENT_TABLE( HostWizard,wxWizard)
  EVT_WIZARD_PAGE_CHANGING( wxID_ANY, HostWizard::OnNextPage )
  EVT_CHECKBOX(wxID_ANY,HostWizard::OnPasswordButton)
END_EVENT_TABLE()


void HostWizard::OnNextPage( wxWizardEvent& event ) {
	if( event.GetDirection() ) 
		if( GetCurrentPage()==sessionPage ) {
			if( sessionNameEdit->GetValue().Length()==0 ) {
		        wxMessageBox(_T("Please enter a session name. Empty session names are not allowed."), _T("Invalid session name"),
							wxICON_ERROR | wxOK, this);
				event.Veto();
			} else {
                
			long ret;
			if( !portEdit->GetValue().ToLong( &ret ) || ret<=0 || ret>65535 ) {
		        wxMessageBox(_T("Please enter a valid port number (between 1 and 65535)."), _T("Invalid port"),
							wxICON_ERROR | wxOK, this);
				event.Veto();			
			}}
		}
}


void HostWizard::OnPasswordButton( wxCommandEvent& event,int index ) {
	if( passwordEnabledCheck->GetValue() ) {
		passwordEdit->Enable();
		st13->Enable();
	} else {
		passwordEdit->Disable();
		st13->Disable();
	}
}


bool HostWizard::TransferDataFromWindow() {
	   return true;
}
