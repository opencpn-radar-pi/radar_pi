/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Authors:  David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2015 by Kees Verruijt         canboat@verruijt.net *
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

#include "RadarCanvas.h"
#include "drawutil.h"

BEGIN_EVENT_TABLE(RadarCanvas, wxGLCanvas)
    EVT_CLOSE(RadarCanvas::close)
    EVT_MOVE(RadarCanvas::moved)
    EVT_SIZE(RadarCanvas::resized)
    EVT_PAINT(RadarCanvas::render)
END_EVENT_TABLE()

// some useful events to use
void RadarCanvas::mouseMoved(wxMouseEvent& event) {}
void RadarCanvas::mouseDown(wxMouseEvent& event) {}
void RadarCanvas::mouseWheelMoved(wxMouseEvent& event) {}
void RadarCanvas::mouseReleased(wxMouseEvent& event) {}
void RadarCanvas::rightClick(wxMouseEvent& event) {}
void RadarCanvas::mouseLeftWindow(wxMouseEvent& event) {}
void RadarCanvas::keyPressed(wxKeyEvent& event) {}
void RadarCanvas::keyReleased(wxKeyEvent& event) {}

static int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0 };

RadarCanvas::RadarCanvas( br24radar_pi * pi, RadarInfo * ri, wxFrame * parent, wxSize size ) :
#if !wxCHECK_VERSION(3,0,0)
    wxGLCanvas( parent, wxID_ANY, wxDefaultPosition, size,
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T(""), attribs )
#else
    wxGLCanvas( parent, wxID_ANY, attribs, wxDefaultPosition, size,
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T("") )
#endif
{
    m_parent = parent;
    m_pi = pi;
    m_ri = ri;
    m_context = new wxGLContext(this);

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

RadarCanvas::~RadarCanvas()
{
    delete m_context;
}

void RadarCanvas::moved( wxMoveEvent& evt )
{
    m_pi->m_dialogLocation[DL_RADARWINDOW + m_ri->radar].pos = m_parent->GetPosition();
}

void RadarCanvas::resized( wxSizeEvent& evt )
{
#if 0
    wxSize s = GetSize();
    wxSize n;
    n.x = MIN(s.x, s.y);
    n.y = s.x;

    if (n.x != s.x || n.y != s.y) {
        SetSize(n);
        //m_parent->Layout();
    }
    m_pi->m_dialogLocation[DL_RADARWINDOW + m_ri->radar].size = n;

    Refresh();
#endif
}

void RadarCanvas::close( wxCloseEvent& evt )
{
    //this->Hide();
    //evt.Skip();
}

void RadarCanvas::prepare2DViewport( int x, int y, int width, int height )
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);   // textures
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glScaled(1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RadarCanvas::render( wxPaintEvent& evt )
{
    int w, h;

    if (!IsShown()) {
        return;
    }

    GetClientSize(&w, &h);

    wxLogMessage(wxT("RadarCanvas: rendering %d by %d"), w, h);

    wxGLCanvas::SetCurrent(*m_context);
    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event

    prepare2DViewport(0, 0, w, h);

    double scale_factor = 1.0 / RETURNS_PER_LINE; // Radar image is in 0..511 range

    DrawOutlineArc(0.25, 1.00, 0.0, 359.0, true);
    DrawOutlineArc(0.50, 0.75, 0.0, 359.0, true);
    CheckOpenGLError();

    // TODO
    // m_pi->RenderGuardZone(wxPoint(0,0), 1.0, 0);
    double rotation = 0.0; // Or HU then -m_pi->m_hdt;


    m_ri->RenderRadarImage(wxPoint(0,0), scale_factor, rotation, false);
    CheckOpenGLError();

    glFlush();
    SwapBuffers();
}

// vim: sw=4:ts=8:
