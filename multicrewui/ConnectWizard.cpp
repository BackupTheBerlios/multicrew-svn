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

#include "ConnectWizard.h"
#include "../multicrewcore/error.h"
#include "../multicrewcore/debug.h"
#include "../multicrewcore/config.h"

#include <wx/thread.h>
#include <deque>


struct ConnectWizard::Data {
	Data( ConnectWizard *wizard ) 
		: setup(), 
		  con( __FILE__, __LINE__ ), 
		  foundHostSlot( &setup.hostFound, wizard, ConnectWizard::OnHostFound  ) {
	}

	ClientConnectionSetup setup;
	SmartPtr<Connection> con;
	typedef SmartPtr<ClientConnectionSetup::FoundHost> SmartFoundHost;
	Slot1<ConnectWizard, SmartFoundHost> foundHostSlot;
	std::deque<SmartFoundHost> hosts;
	wxCriticalSection sessionsCritSec;
};


ConnectWizard::ConnectWizard(wxWindow* parent,wxWindowID id,const wxString& title,const wxBitmap& bitmap,const wxPoint& pos)
  VwX_INIT_OBJECTS_ConnectWizard 
{
	d = new Data( this );

	wxBitmap m_bitmap=bitmap;
	if( m_bitmap==wxNullBitmap ) {
		m_bitmap= wxBitmap( "BMP_CONNECTWIZARD", wxBITMAP_TYPE_BMP_RESOURCE );
	}
	Create( parent, id, title, m_bitmap, pos );
	VwXinit();
	port->SetValue( Config::config()->stringValue( "", "DefaultPort", MULTICREW_PORT ).c_str() );
	hostName->SetLabel(wxT("localhost"));
	Refresh();
}

ConnectWizard::~ConnectWizard() {
 	delete st33;
 	delete st34;
 	delete hostName;
 	delete port;
 	delete hostNameStatic;
 	delete st38;
 	delete sessions;
 	delete st41;
 	delete lno46;
 	delete broadcastRadio;
 	delete directRadio;
	delete startPage;
 	delete hostPage;
	delete sessionlistPage;

	dout << "> ~ConnectWizard" << std::endl;
	d->sessionsCritSec.Enter();
	d->hosts.clear();
	d->sessionsCritSec.Leave();
	dout << "< ~ConnectWizard" << std::endl;
	delete d;
}

void ConnectWizard::VwXinit() {
 SetTitle(wxT("Multicrew Connection Wizard"));
 startPage=new wxWizardPageSimple(this);
   startPage->SetSize(139,12,318,250);
   FitToPage(startPage);
 hostPage=new wxWizardPageSimple(this);
   hostPage->SetSize(139,12,328,250);
   FitToPage(hostPage);
 st33=new wxStaticText(startPage,-1,wxT(""),wxPoint(0,0),wxSize(390,323),wxST_NO_AUTORESIZE);
   st33->SetLabel(wxT("static text"));
 st34=new wxStaticText(hostPage,-1,wxT(""),wxPoint(5,0),wxSize(325,68),wxST_NO_AUTORESIZE);
   st34->SetLabel(wxT("Search for games in the local network or enter a specific address. This can be a raw IP address (e.g. 192.168.0.7) or a fully qualified hostname with domain (e.g. multicrew.homeip.net). The port number normally does not to be changed. It must match though with the port that the host uses."));
 hostName=new wxTextCtrl(hostPage,-1,wxT(""),wxPoint(150,136),wxSize(160,21));
   hostName->Enable(false);
   hostName->SetLabel(wxT("Text"));
 port=new wxTextCtrl(hostPage,-1,wxT(""),wxPoint(55,190),wxSize(45,21));
   port->SetLabel(wxT("Text"));
   port->Enable(false);
 hostNameStatic=new wxStaticText(hostPage,-1,wxT(""),wxPoint(10,140),wxSize(135,13),wxST_NO_AUTORESIZE);
   hostNameStatic->Enable(false);
   hostNameStatic->SetLabel(wxT("Hostname or IP address:"));
 st38=new wxStaticText(hostPage,-1,wxT(""),wxPoint(10,194),wxSize(45,13),wxST_NO_AUTORESIZE);
   st38->SetLabel(wxT("Port:"));
 sessionlistPage=new wxWizardPageSimple(this);
   sessionlistPage->SetSize(129,12,328,250);
   FitToPage(sessionlistPage);
 sessions=new wxListBox(sessionlistPage,-1,wxPoint(0,15),wxSize(320,93),0,NULL, wxLB_SINGLE);
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
 //Refresh();
}

SmartPtr<Connection> ConnectWizard::RunWizard() {
	// init connection object
	bool ret = d->setup.init();
	if( !ret ) {
		derr << "Initialization of DirectPlay failed. Take a look at the log to find out why." << std::endl;
		return 0;
	}

	// run wizard
	ret = wxWizard::RunWizard(startPage);
	if( ret )
		return d->con;
	else
		return 0;
}
 
BEGIN_EVENT_TABLE(ConnectWizard,wxWizard)
	EVT_WIZARD_PAGE_CHANGING( wxID_ANY, ConnectWizard::OnNextPage )
	EVT_WIZARD_PAGE_CHANGED( wxID_ANY, ConnectWizard::OnPageChanged )
	EVT_RADIOBUTTON( wxID_ANY, ConnectWizard::OnRadioButton	)
END_EVENT_TABLE()

	
void ConnectWizard::OnNextPage( wxWizardEvent& event ) {
	dout << "> OnNextPage" << std::endl;
	if( event.GetDirection() ) {
		if( GetCurrentPage()==hostPage ) {
			wxIPV4address addr;
			if( directRadio->GetValue() && addr.Hostname( hostName->GetValue() )==false ) {
		        wxMessageBox(_T("Please enter a valid internet (IP v4) address for the hostname."), _T("Invalid hostname"),
							 wxICON_ERROR | wxOK, this);
				event.Veto();
				return;
			}
                
			if( addr.Service( port->GetValue() )==false ) {
				wxMessageBox(_T("Please enter a valid port number (between 1 and 65535)."), 
							 _T("Invalid port"),
							 wxICON_ERROR | wxOK, this);
				event.Veto();
				return;
			}
		} else
		if( GetCurrentPage()==sessionlistPage ) {
			d->setup.stopSearch();

			// session selected?
			int selected = sessions->GetSelection();
			if( selected==wxNOT_FOUND ) {
				wxMessageBox(_T("Please select the host you want to connect to."), 
							 _T("No host selected"),
							 wxICON_ERROR | wxOK, this);
				event.Veto();
				return;
			}

			// connect finally
			d->con = d->setup.connect( d->hosts[selected] );
			if( d->con.isNull() ) {
				derr << "Connection failed. Look at the log messages to find out why." 
					 << std::endl;
				event.Veto();
				return;
			}
		}
	}	
	dout << "< OnNextPage" << std::endl;
}


void ConnectWizard::OnPageChanged( wxWizardEvent& event ) {
	dout << "> OnPageChanged" << std::endl;
	if( event.GetDirection() ) {
		if( GetCurrentPage()==sessionlistPage ) {
			// just in case
			dout << "stop search" << std::endl;
			d->setup.stopSearch();

			// clear host search list
			dout << "clear list" << std::endl;
			d->sessionsCritSec.Enter();
			d->hosts.clear();
			sessions->Clear();
			sessions->Refresh();
			d->sessionsCritSec.Leave();

			// start new host search
			dout << "start new search" << std::endl;
			long portNum;
			port->GetValue().ToLong( &portNum );

			bool ret = d->setup.startSearch( broadcastRadio->GetValue(),
											 hostName->GetValue().c_str(), portNum );
			if( !ret )
				derr << "Search failed. Take a look at the logs to find out why." << std::endl;
		}
	}

	dout << "< OnPageChanged" << std::endl;
}


void ConnectWizard::OnHostFound( SmartPtr<ClientConnectionSetup::FoundHost> foundHost ) {
	// insert host into the list
	d->sessionsCritSec.Enter();
	d->hosts.push_back( foundHost );
	sessions->Insert( (foundHost->description().c_str() + 
			wxString::Format(" (%i ms)", foundHost->latency()) ).c_str(),
		sessions->GetCount(), &foundHost );
	sessions->Refresh();
	d->sessionsCritSec.Leave();
}


void ConnectWizard::OnRadioButton( wxCommandEvent& event ) {
	if( event.m_eventObject==directRadio ) {
		hostName->Enable();
		port->Enable();
	} else

	if( event.m_eventObject==broadcastRadio ) {
		hostName->Disable();
		port->Disable();
	}
}
