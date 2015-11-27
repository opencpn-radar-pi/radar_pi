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

#include <br24radar_pi.h>

BEGIN_EVENT_TABLE(RadarWindow, wxGLCanvas)
    EVT_CLOSE(RadarWindow::close)
    EVT_SIZE(RadarWindow::resized)
    EVT_PAINT(RadarWindow::render)
END_EVENT_TABLE()

// some useful events to use
void RadarWindow::mouseMoved(wxMouseEvent& event) {}
void RadarWindow::mouseDown(wxMouseEvent& event) {}
void RadarWindow::mouseWheelMoved(wxMouseEvent& event) {}
void RadarWindow::mouseReleased(wxMouseEvent& event) {}
void RadarWindow::rightClick(wxMouseEvent& event) {}
void RadarWindow::mouseLeftWindow(wxMouseEvent& event) {}
void RadarWindow::keyPressed(wxKeyEvent& event) {}
void RadarWindow::keyReleased(wxKeyEvent& event) {}

static int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0 };

RadarWindow::RadarWindow( RadarInfo * ri, wxFrame * parent, int * args ) :
#if !wxCHECK_VERSION(3,0,0)
    wxGLCanvas( parent, wxID_ANY, wxDefaultPosition, wxSize(256, 256),
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T(""), attribs )
#else
    wxGLCanvas( parent, wxID_ANY, attribs, wxDefaultPosition, wxSize(256, 256),
                wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T("") )
#endif
{
    m_ri = ri;
    m_context = new wxGLContext(this);

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

RadarWindow::~RadarWindow()
{
    delete m_context;
}

void RadarWindow::resized( wxSizeEvent& evt )
{
    Refresh();
}

void RadarWindow::close( wxCloseEvent& evt )
{
    this->Hide();
}

void RadarWindow::prepare2DViewport( int x, int y, int width, int height )
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

int RadarWindow::getWidth()
{
    return GetSize().x;
}

int RadarWindow::getHeight()
{
    return GetSize().y;
}

void RadarWindow::render( wxPaintEvent& evt )
{
    int w, h;

    if (!IsShown()) {
        return;
    }

    GetClientSize(&w, &h);

    wxLogMessage(wxT("RadarWindow: rendering %d by %d"), w, h);

    wxGLCanvas::SetCurrent(*m_context);
    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event

    prepare2DViewport(0, 0, w, h);

    double scale_factor = 1.0 / RETURNS_PER_LINE; // Radar image is in 0..511 range

    // TODO
    // m_pi->RenderGuardZone(wxPoint(0,0), 1.0, 0);
    double rotation = 0.0; // Or HU then -m_pi->m_hdt;
    if (m_ri->draw) {
        m_ri->draw->DrawRadarImage(wxPoint(0,0), scale_factor, rotation, false);
    }

    glFlush();
    SwapBuffers();
}

