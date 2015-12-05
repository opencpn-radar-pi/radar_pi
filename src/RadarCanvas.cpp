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
#include "TextureFont.h"

BEGIN_EVENT_TABLE(RadarCanvas, wxGLCanvas)
//    EVT_CLOSE(RadarCanvas::close)
//    EVT_MOVE(RadarCanvas::moved)
//    EVT_SIZE(RadarCanvas::resized)
    EVT_PAINT(RadarCanvas::render)
END_EVENT_TABLE()

static int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0 };

RadarCanvas::RadarCanvas( br24radar_pi * pi, RadarInfo * ri, wxWindow * parent, wxSize size ) :
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

#if 0
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

// some useful events to use
void RadarCanvas::mouseMoved(wxMouseEvent& event) {}
void RadarCanvas::mouseDown(wxMouseEvent& event) {}
void RadarCanvas::mouseWheelMoved(wxMouseEvent& event) {}
void RadarCanvas::mouseReleased(wxMouseEvent& event) {}
void RadarCanvas::rightClick(wxMouseEvent& event) {}
void RadarCanvas::mouseLeftWindow(wxMouseEvent& event) {}
void RadarCanvas::keyPressed(wxKeyEvent& event) {}
void RadarCanvas::keyReleased(wxKeyEvent& event) {}
#endif

void RadarCanvas::render( wxPaintEvent& evt )
{
    int w, h;

    if (!IsShown()) {
        return;
    }
    if (!m_pi->m_opencpn_gl_context) {
        return;
    }

    GetClientSize(&w, &h);

    wxLogMessage(wxT("RadarCanvas: rendering %d by %d"), w, h);

    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event
    SetCurrent(*m_context);

    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    wxFont font( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    m_FontNormal.Build(font);
    wxFont bigFont( 24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );
    m_FontBig.Build(bigFont);

    glClearColor(0.0f, 0.1f, 0.0f, 1.0f);               // Black Background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the canvas
    glEnable(GL_TEXTURE_2D);                            // Enable textures
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_BLEND);
    // glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);                        // Next two operations on the project matrix stack
    glLoadIdentity();                                   // Reset projection matrix stack
    glScaled(1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);                         // Reset matrick stack target back to GL_MODELVIEW

    double scale_factor = 1.0 / RETURNS_PER_LINE; // Radar image is in 0..511 range

    glColor3ub(200, 255, 200);
    DrawOutlineArc(0.25, 1.00, 0.0, 359.0, true);
    DrawOutlineArc(0.50, 0.75, 0.0, 359.0, true);
    CheckOpenGLError(wxT("range circles"));

    // TODO
    // m_pi->RenderGuardZone(wxPoint(0,0), 1.0, 0);
    double rotation = 0.0; // Or HU then -m_pi->m_hdt;

    m_ri->RenderRadarImage(wxPoint(0,0), scale_factor, rotation, false);
    CheckOpenGLError(wxT("radar image"));

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);                        // Next two operations on the project matrix stack
    glLoadIdentity();                                   // Reset projection matrix stack
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);                         // Reset matrick stack target back to GL_MODELVIEW

    glColor3ub(200, 255, 200);
    glEnable(GL_TEXTURE_2D);

    wxString s = m_ri->GetCanvasText();
    m_FontBig.RenderString(s, 0, 0);
    CheckOpenGLError(wxT("font render"));
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);                        // Next two operations on the project matrix stack
    glLoadIdentity();                                   // Reset projection matrix stack
    glMatrixMode(GL_MODELVIEW);                         // Reset matrick stack target back to GL_MODELVIEW

    glPopAttrib();
    glPopMatrix();
    glFlush();
    CheckOpenGLError(wxT("finalize"));
    glFinish();
    SwapBuffers();

    SetCurrent(*m_pi->m_opencpn_gl_context);
}

// vim: sw=4:ts=8:
