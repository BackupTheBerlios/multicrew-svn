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

#include "StatusDialog.h"
#include "../multicrewcore/multicrewcore.h"
#include "../multicrewcore/signals.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>

struct StatusDialog::Data {
	Data( StatusDialog *dlg )
		: core( MulticrewCore::multicrewCore() ) {}

	wxTextCtrl *logs;
	wxStaticText *st4;
	wxCheckBox *hostConnected;
	wxStaticText *st9;

	SmartPtr<MulticrewCore> core;
};

StatusDialog::StatusDialog(wxWindow* parent,wxWindowID id,const wxString& title,const wxPoint& pos,const wxSize& size,long style,const wxString& name)
  VwX_INIT_OBJECTS_StatusDialog
{
	d = new Data( this );
	
	Create(parent,id,title,pos,size,style,name);

	if((pos==wxDefaultPosition)&&(size==wxDefaultSize)){
		SetSize(0,0,390,315);
	}

	if((pos!=wxDefaultPosition)&&(size==wxDefaultSize)){
		SetSize(390,315);
	}
	VwXinit();
}

StatusDialog::~StatusDialog() {
	delete d->logs;
	delete d->st4;
	delete d->hostConnected;
	delete d->st9;
	delete d;
}

void StatusDialog::VwXinit() {
 SetTitle(wxT("Multicrew Status"));
 d->logs=new wxTextCtrl(this,-1,wxT(""),wxPoint(10,85),wxSize(365,193),wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
 d->st4=new wxStaticText(this,-1,wxT(""),wxPoint(10,65),wxSize(365,13),wxST_NO_AUTORESIZE);
   d->st4->SetLabel(wxT("Log output:"));
 d->st9=new wxStaticText(this,-1,wxT(""),wxPoint(40,40),wxSize(340,13),wxST_NO_AUTORESIZE);
   d->st9->SetLabel(wxT("Connected to host"));
 d->hostConnected=new wxCheckBox(this,-1,wxT(""),wxPoint(10,40),wxSize(13,13));
   d->hostConnected->Enable(false); 
 Refresh();
}
 
BEGIN_EVENT_TABLE( StatusDialog,wxDialog)
END_EVENT_TABLE()


void StatusDialog::log( std::string line ) {
	d->logs->AppendText( line.c_str() );
}


void StatusDialog::setConnected() {
	d->hostConnected->SetValue( true );
}


void StatusDialog::setUnconnected() {
	d->hostConnected->SetValue( false );
}
