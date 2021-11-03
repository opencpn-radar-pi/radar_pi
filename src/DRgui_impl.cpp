/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  DR Plugin
 * Author:   Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2013 by Mike Rossiter                                   *
 *   $EMAIL$                                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include "DRgui_impl.h"
#include "DRgui.h"
#include "DR_pi.h"
#include "icons.h"

#include <wx/progdlg.h>
#include <wx/wx.h>
#include "wx/dir.h"
#include <list>
#include <cmath>

#ifdef __OCPN__ANDROID__
wxWindow *g_Window;
#endif

#define FAIL(X) do { error = X; goto failed; } while(0)

Dlg::Dlg(wxWindow *parent, DR_pi *ppi)
	: m_Dialog(parent)
{
	this->Fit();
	dbg = false; //for debug output set to true
	
	pPlugIn = ppi;
	pParent = parent;

	wxString blank_name = *GetpSharedDataLocation()
		+ _T("plugins/DR_pi/data/blank.ico");

	wxIcon icon(blank_name, wxBITMAP_TYPE_ICO);
	SetIcon(icon);

#ifdef __OCPN__ANDROID__
    g_Window = this;
    GetHandle()->setStyleSheet( qtStyleSheet);
    Connect( wxEVT_MOTION, wxMouseEventHandler( Dlg::OnMouseEvent ) );
#endif
	
}

Dlg::~Dlg()
{
	
}

#ifdef __OCPN__ANDROID__ 
wxPoint g_startPos;
wxPoint g_startMouse;
wxPoint g_mouse_pos_screen;

void Dlg::OnMouseEvent( wxMouseEvent& event )
{
    g_mouse_pos_screen = ClientToScreen( event.GetPosition() );
    
    if(event.Dragging()){
        int x = wxMax(0, g_startPos.x + (g_mouse_pos_screen.x - g_startMouse.x));
        int y = wxMax(0, g_startPos.y + (g_mouse_pos_screen.y - g_startMouse.y));
        int xmax = ::wxGetDisplaySize().x - GetSize().x;
        x = wxMin(x, xmax);
        int ymax = ::wxGetDisplaySize().y - (GetSize().y * 2);          // Some fluff at the bottom
        y = wxMin(y, ymax);
        
        g_Window->Move(x, y);
    }
}
#endif

void Dlg::Addpoint(TiXmlElement* Route, wxString ptlat, wxString ptlon, wxString ptname, wxString ptsym, wxString pttype){
//add point
	TiXmlElement * RoutePoint = new TiXmlElement( "rtept" );
    RoutePoint->SetAttribute("lat", ptlat.mb_str());
    RoutePoint->SetAttribute("lon", ptlon.mb_str());


    TiXmlElement * Name = new TiXmlElement( "name" );
    TiXmlText * text = new TiXmlText( ptname.mb_str() );
    RoutePoint->LinkEndChild( Name );
    Name->LinkEndChild( text );

    TiXmlElement * Symbol = new TiXmlElement( "sym" );
    TiXmlText * text1 = new TiXmlText( ptsym.mb_str() );
    RoutePoint->LinkEndChild( Symbol );
    Symbol->LinkEndChild( text1 );

    TiXmlElement * Type = new TiXmlElement( "type" );
    TiXmlText * text2 = new TiXmlText( pttype.mb_str() );
    RoutePoint->LinkEndChild( Type );
    Type->LinkEndChild( text2 );
    Route->LinkEndChild( RoutePoint );
//done adding point
}

void Dlg::OnPSGPX( wxCommandEvent& event )
{
   Calculate(event, true, 1);
}

void Dlg::OnClose(wxCloseEvent& event)
{	
	pPlugIn->OnDRDialogClose();
}

bool Dlg::OpenXML()
{
    Position my_position;
	
    my_positions.clear();
	
	int response = wxID_CANCEL;
	int my_count = 0;
    
	wxArrayString file_array;
    wxString filename;

	if (m_gpx_path == wxEmptyString) {
		wxFileName fn;
		wxString tmp_path;

		tmp_path = GetPluginDataDir("DR_pi");
		fn.SetPath(tmp_path);
		fn.AppendDir(_T("data"));

		m_gpx_path = fn.GetFullPath();
	}

	wxFileDialog openDialog( this, _( "Import GPX Route file" ), m_gpx_path, wxT ( "" ),
                wxT ( "GPX files (*.gpx)|*.gpx|All files (*.*)|*.*" ),
                wxFD_OPEN | wxFD_MULTIPLE );
        response = openDialog.ShowModal();
        if( response == wxID_OK ) {
            openDialog.GetPaths( file_array );

            //    Record the currently selected directory for later use
            if( file_array.GetCount() ) {
                wxFileName fn( file_array[0] );
				filename = file_array[0];
                m_gpx_path = fn.GetPath();
            }
        }
		else if(response == wxID_CANCEL){
		return false;
		}
    
    TiXmlDocument doc;
    wxString error;
    wxProgressDialog *progressdialog = NULL;

    
	if(!doc.LoadFile(filename.mb_str())){
        FAIL(_("Failed to load file: ") + filename);
	}
    else {
        TiXmlElement *root = doc.RootElement();
        if(!strcmp(root->Value(), "rte"))
            FAIL(_("rte Invalid xml file"));

        int count = 0;
        for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement())
            count++;

        int i=0;
        for(TiXmlElement* e = root->FirstChildElement(); e; e = e->NextSiblingElement(), i++) {
            if(progressdialog) {
                if(!progressdialog->Update(i))
                    return true;
            } else {
               if(1) {
                    progressdialog = new wxProgressDialog(
                        _("Route"), _("Loading"), count, this,
                        wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME);
                }
            }
              				
                for(TiXmlElement* f = e->FirstChildElement(); f; f = f->NextSiblingElement()) {
                    if(!strcmp(f->Value(), "rtept")) {
                        wxString rte_lat = wxString::FromUTF8(f->Attribute("lat"));
						wxString rte_lon = wxString::FromUTF8(f->Attribute("lon"));
                        		
						my_position.lat = rte_lat;
						my_position.lon = rte_lon;
						my_positions.push_back(my_position);					   
					}  //else if(!strcmp(f->Value(), "extensions")) {
                        //rte_start = wxString::FromUTF8(f->Attribute("opencpn:start"));
						//rte_end = wxString::FromUTF8(f->Attribute("opencpn:end"));

                    //}				
                }

        }
    }

    delete progressdialog;
    return true;

failed:
    delete progressdialog;

    wxMessageDialog mdlg(this, error, _("DR"), wxOK | wxICON_ERROR);
    mdlg.ShowModal();

    return false;
}


void Dlg::Calculate( wxCommandEvent& event, bool write_file, int Pattern  ){
   if(OpenXML()){
  
	bool error_occured=false;
   // double dist, fwdAz, revAz;


    double lat1,lon1;
   // if(!this->m_Lat1->GetValue().ToDouble(&lat1)){ lat1=0.0;}
   // if(!this->m_Lon1->GetValue().ToDouble(&lon1)){ lon1=0.0;}
	int num_hours;

	num_hours = this->m_Nship->GetSelection();

	wxString defaultFileName;
    defaultFileName = this->m_Route->GetValue();
	
	lat1 = 0.0;
	lon1 = 0.0;
    //if (error_occured) wxMessageBox(_T("error in conversion of input coordinates"));

    //error_occured=false;
    wxString s;
    if (write_file){
        wxFileDialog dlg(this, _("Export DR Positions in GPX file as"), wxEmptyString, defaultFileName, _T("GPX files (*.gpx)|*.gpx|All files (*.*)|*.*"), wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() == wxID_CANCEL){
            error_occured=true;     // the user changed idea...
		    return;
		}
			
		//dlg.ShowModal();
        s=dlg.GetPath();
        //  std::cout<<s<< std::endl;
        if (dlg.GetPath() == wxEmptyString){ error_occured=true; if (dbg) printf("Empty Path\n");}
    }

    //Validate input ranges
    if (!error_occured){
        if (std::abs(lat1)>90){ error_occured=true; }
        if (std::abs(lon1)>180){ error_occured=true; }
        if (error_occured) wxMessageBox(_("error in input range validation"));
    }

    //Start GPX
    TiXmlDocument doc;
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "utf-8", "" );
    doc.LinkEndChild( decl );
    TiXmlElement * root = new TiXmlElement( "gpx" );
    TiXmlElement * Route = new TiXmlElement( "rte" );
    TiXmlElement * RouteName = new TiXmlElement( "name" );
    TiXmlText * text4 = new TiXmlText( this->m_Route->GetValue().ToUTF8() );
	TiXmlText * textSpeed = new TiXmlText(this->m_Speed_PS->GetValue().ToUTF8());
	

    if (write_file){
        doc.LinkEndChild( root );
        root->SetAttribute("version", "0.1");
        root->SetAttribute("creator", "DR_pi by Rasbats");
        root->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
        root->SetAttribute("xmlns:gpxx","http://www.garmin.com/xmlschemas/GpxExtensions/v3" );
        root->SetAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
        root->SetAttribute("xmlns:opencpn","http://www.opencpn.org");
        Route->LinkEndChild( RouteName );
        RouteName->LinkEndChild( text4 );


        TiXmlElement * Extensions = new TiXmlElement( "extensions" );

        TiXmlElement * StartN = new TiXmlElement( "opencpn:start" );
        TiXmlText * text5 = new TiXmlText( "Start" );
        Extensions->LinkEndChild( StartN );
        StartN->LinkEndChild( text5 );

        TiXmlElement * EndN = new TiXmlElement( "opencpn:end" );
        TiXmlText * text6 = new TiXmlText( "End" );
        Extensions->LinkEndChild( EndN );
        EndN->LinkEndChild( text6 );

		TiXmlElement * Speed = new TiXmlElement("opencpn:planned_speed");
		Extensions->LinkEndChild(Speed);	
		Speed->LinkEndChild(textSpeed);

        Route->LinkEndChild( Extensions );
    }

    switch ( Pattern ) {
    case 1:
        {		
        if (dbg) cout<<"DR Calculation\n";      
        double speed=5;
		int    interval=1;
        		
		if(!this->m_Speed_PS->GetValue().ToDouble(&speed)){ speed=5.0;} // 5 kts default speed
		interval = m_Nship->GetCurrentSelection();//S=1		

		interval += 1;

		speed = speed*(double)interval;

        int n=0;
        //int multiplier=1;
        double lati, loni;
        double latN[100], lonN[100];
		double latF, lonF;
		
		Position my_point;		
       
		double value, value1;
		
		for(std::vector<Position>::iterator it = my_positions.begin();  it != my_positions.end(); it++){
       
			if(!(*it).lat.ToDouble(&value)){ /* error! */ }
				lati = value;
			if(!(*it).lon.ToDouble(&value1)){ /* error! */ }
				loni = value1;

			latN[n] = lati;
			lonN[n] = loni;

			n++;//0,1,2,3
		}
		
		my_positions.clear();

		n--;//n = 2,  0,1,2
		int routepoints = n+1; //3
		
		
		double myDist, myBrng, myDistForBrng;
		int count_pts;
		double remaining_dist, myLast, route_dist;
		remaining_dist = 0;
		route_dist= 0;
		myLast = 0;
		myDistForBrng =0;
		double total_dist = 0;
		int i,c;
		bool skip = false;
		bool inloop = false;
		bool setF = false;

		latF = latN[0];
		lonF = lonN[0];

		// Start of new logic
		for (i=0; i<n; i++){	// n is number of routepoints		

			// save the routepoint
			my_point.lat = wxString::Format(wxT("%f"),latN[i]);
			my_point.lon = wxString::Format(wxT("%f"),lonN[i]);
			my_point.routepoint = 1;
			my_point.wpt_num =  wxString::Format(wxT("%d"),(int)i);
			my_points.push_back(my_point);	
            			
			if (i==0){ // First F is a routepoint
				latF = latN[i];
				lonF = lonN[i];
			}

			DistanceBearingMercator(latN[i + 1], lonN[i + 1],latF, lonF,  &myDist, &myBrng);

			total_dist = total_dist + myDist; // 

			if (total_dist > speed){	
				//
				// DR point is before the next route point
				//
				route_dist = total_dist - myDist;	// route_dist is the distance between the previous DR and the route point
				remaining_dist = speed - route_dist;// distance between route point and next DR

				DistanceBearingMercator(latN[i + 1], lonN[i + 1], latN[i], lonN[i], &myDist, &myBrng);
				destLoxodrome(latN[i], lonN[i], myBrng, remaining_dist, &lati, &loni);

				// Put in the DR point after the route point
				my_point.lat = wxString::Format(wxT("%f"), lati);
				my_point.lon = wxString::Format(wxT("%f"), loni);
				my_point.routepoint = 0;
				my_points.push_back(my_point);

				latF = lati;
				lonF = loni;

				total_dist = 0;

				// 
				// 
				DistanceBearingMercator(latN[i + 1], lonN[i + 1], latF, lonF, &myDistForBrng, &myBrng);// Distance between route points

				if (myDistForBrng > speed) {
					// 
					//
					// put in the DR positions
					//
					count_pts = (int)floor(myDistForBrng / speed);
					//
					remaining_dist = myDistForBrng - (count_pts*speed);
					DistanceBearingMercator(latN[i + 1], lonN[i + 1], latF, lonF, &myDistForBrng, &myBrng);

					for (c = 1; c <= count_pts; c++) {
						destLoxodrome(latF, lonF, myBrng, speed*c, &lati, &loni);
						// print mid points
						my_point.lat = wxString::Format(wxT("%f"), lati);
						my_point.lon = wxString::Format(wxT("%f"), loni);
						my_point.routepoint = 0;
						my_points.push_back(my_point);
						//	End of prints					
					}

					latF = lati;
					lonF = loni;

					total_dist = 0;
					//
					//
					// All the DR positions inserted for this leg
				}

				if (total_dist == 0) {
					DistanceBearingMercator(latN[i + 1], lonN[i + 1], latF, lonF, &myDistForBrng, &myBrng);
					total_dist = myDistForBrng; // distance between DR and the next route point
					latF = latN[i + 1];
					lonF = lonN[i + 1];
				}
			
			}															
			else{
				//
				latF = latN[i+1];
				lonF = lonN[i+1];
				//
				//
				//
				//
			}   //			

		}
		// End of new logic
		// print the last routepoint
		my_point.lat = wxString::Format(wxT("%f"),latN[i]);
		my_point.lon = wxString::Format(wxT("%f"),lonN[i]);
		my_point.routepoint = 1;
		my_point.wpt_num =  wxString::Format(wxT("%d"),(int)i);
		my_points.push_back(my_point);
		//


		for(std::vector<Position>::iterator itOut = my_points.begin();  itOut != my_points.end(); itOut++){
			//wxMessageBox((*it).lat, _T("*it.lat"));
		
        double value, value1;
		if(!(*itOut).lat.ToDouble(&value)){ /* error! */ }
			lati = value;
		if(!(*itOut).lon.ToDouble(&value1)){ /* error! */ }
			loni = value1;
		
		if ((*itOut).routepoint == 1){
			if (write_file){Addpoint(Route, wxString::Format(wxT("%f"),lati), wxString::Format(wxT("%f"),loni), (*itOut).wpt_num ,_T("diamond"),_T("WPT"));}
		}
		else{
			if ((*itOut).routepoint == 0){
				if (write_file){Addpoint(Route, wxString::Format(wxT("%f"),lati), wxString::Format(wxT("%f"),loni), _T("DR") ,_T("square"),_T("WPT"));}			
			}
		}
        
		}
		
		my_points.clear();		
        break;
		
		}

    
      default:
      {            // Note the colon, not a semicolon
        cout<<"Error, bad input, quitting\n";
        break;
      }
    }

       if (write_file){
            root->LinkEndChild( Route );
            // check if string ends with .gpx or .GPX
			if (!s.EndsWith(_T(".gpx"))) {
                 s = s + _T(".gpx");
            }
            wxCharBuffer buffer=s.ToUTF8();
            if (dbg) std::cout<< buffer.data()<<std::endl;
            doc.SaveFile( buffer.data() );}
    //} //end of if no error occured

    if (error_occured==true)  {
        wxLogMessage(_("Error in calculation. Please check input!") );
        wxMessageBox(_("Error in calculation. Please check input!") );
    }
  }
}
