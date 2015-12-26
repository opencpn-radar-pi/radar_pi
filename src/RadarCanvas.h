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

#ifndef _RADAR_CANVAS_H_
#define _RADAR_CANVAS_H_

#include "br24radar_pi.h"
#include "TextureFont.h"

class RadarCanvas : public wxGLCanvas {
  wxGLContext* m_context;

 public:
  RadarCanvas(br24radar_pi* pi, RadarInfo* ri, wxWindow* parent, wxSize size);
  virtual ~RadarCanvas();

  void Render(wxPaintEvent& evt);
  void OnMove(wxMoveEvent& evt);
  void OnSize(wxSizeEvent& evt);
  void OnMouseClick(wxMouseEvent& event);

#if 0
    // events
    void moved(wxMoveEvent& evt);
    void close(wxCloseEvent& evt);
    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);
    void keyPressed(wxKeyEvent& event);
    void keyReleased(wxKeyEvent& event);
#endif

 private:
  void RenderText(wxPoint p, wxString text);

  wxWindow* m_parent;
  br24radar_pi* m_pi;
  RadarInfo* m_ri;

  TextureFont m_FontNormal;
  TextureFont m_FontBig;

  DECLARE_EVENT_TABLE();
};

#endif
