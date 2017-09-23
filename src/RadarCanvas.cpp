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
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
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
#include "TextureFont.h"
#include "drawutil.h"

PLUGIN_BEGIN_NAMESPACE

BEGIN_EVENT_TABLE(RadarCanvas, wxGLCanvas)
EVT_MOVE(RadarCanvas::OnMove)
EVT_SIZE(RadarCanvas::OnSize)
EVT_PAINT(RadarCanvas::Render)
EVT_MOUSEWHEEL(RadarCanvas::OnMouseWheel)
EVT_LEFT_DOWN(RadarCanvas::OnMouseClick)
END_EVENT_TABLE()

static int attribs[] = {WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, WX_GL_STENCIL_SIZE, 8, 0};

RadarCanvas::RadarCanvas(br24radar_pi *pi, RadarInfo *ri, wxWindow *parent, wxSize size)
    : wxGLCanvas(parent, wxID_ANY, attribs, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T("")) {
  m_parent = parent;
  m_pi = pi;
  m_ri = ri;
  m_context = new wxGLContext(this);
  m_zero_context = new wxGLContext(this);
  m_cursor_texture = 0;
  m_last_mousewheel_zoom_in = 0;
  m_last_mousewheel_zoom_out = 0;

  LOG_VERBOSE(wxT("BR24radar_pi: %s create OpenGL canvas"), m_ri->m_name.c_str());
  Refresh(false);
}

RadarCanvas::~RadarCanvas() {
  LOG_VERBOSE(wxT("BR24radar_pi: %s destroy OpenGL canvas"), m_ri->m_name.c_str());
  delete m_context;
  delete m_zero_context;
  if (m_cursor_texture) {
    glDeleteTextures(1, &m_cursor_texture);
    m_cursor_texture = 0;
  }
}

void RadarCanvas::OnSize(wxSizeEvent &evt) {
  wxSize parentSize = m_parent->GetSize();
  LOG_DIALOG(wxT("BR24radar_pi: %s resize OpenGL canvas to %d, %d"), m_ri->m_name.c_str(), parentSize.x, parentSize.y);
  Refresh(false);
  if (GetSize() != parentSize) {
    SetSize(parentSize);
  }
}

void RadarCanvas::OnMove(wxMoveEvent &evt) {
  wxPoint pos = m_parent->GetPosition();
  LOG_DIALOG(wxT("BR24radar_pi: %s move OpenGL canvas to %d, %d"), m_ri->m_name.c_str(), pos.x, pos.y);
}

void RadarCanvas::RenderTexts(int w, int h) {
  int x, y;

  wxString s;

#define MENU_ROUNDING 4
#define MENU_BORDER 8
#define MENU_EXTRA_WIDTH 32

  // Draw Menu in the top right

  s = _("Menu");
  m_FontMenu.GetTextExtent(s, &x, &y);

  // Calculate the size of the rounded rect, this is also where you can 'click'...
  m_menu_size.x = x + 2 * (MENU_BORDER + MENU_EXTRA_WIDTH);
  m_menu_size.y = y + 2 * (MENU_BORDER);

  glColor4ub(40, 40, 100, 128);

  DrawRoundRect(w - m_menu_size.x, 0, m_menu_size.x, m_menu_size.y, 4);

  glColor4ub(100, 255, 255, 255);
  // The Menu text is slightly inside the rect
  m_FontMenu.RenderString(s, w - m_menu_size.x + MENU_BORDER + MENU_EXTRA_WIDTH, MENU_BORDER);

  // Draw - + in mid bottom

  s = wxT("  -   + ");
  m_FontMenuBold.GetTextExtent(s, &x, &y);

  // Calculate the size of the rounded rect, this is also where you can 'click'...
  m_zoom_size.x = x + 2 * (MENU_BORDER);
  m_zoom_size.y = y + 2 * (MENU_BORDER);

  glColor4ub(80, 80, 80, 128);

  DrawRoundRect(w / 2 - m_zoom_size.x / 2, h - m_zoom_size.y + MENU_ROUNDING, m_zoom_size.x, m_zoom_size.y, MENU_ROUNDING);

  glColor4ub(200, 200, 200, 255);
  // The Menu text is slightly inside the rect
  m_FontMenuBold.RenderString(s, w / 2 - m_zoom_size.x / 2 + MENU_BORDER, h - m_zoom_size.y + MENU_BORDER);

  glColor4ub(200, 255, 200, 255);

  s = m_ri->GetCanvasTextTopLeft();
  m_FontBig.RenderString(s, 0, 0);

  s = m_ri->GetCanvasTextBottomLeft();
  if (s.length()) {
    m_FontBig.GetTextExtent(s, &x, &y);
    m_FontBig.RenderString(s, 0, h - y);
  }

  s = m_ri->GetCanvasTextCenter();
  if (s.length()) {
    m_FontBig.GetTextExtent(s, &x, &y);
    m_FontBig.RenderString(s, (w - x) / 2, (h - y) / 2);
  }
}

void RadarCanvas::RenderRangeRingsAndHeading(int w, int h) {
  // Max range ringe
  float r = wxMax(w, h) / 2.0;

  // Position of the range texts
  float x = sinf((float)(0.25 * PI)) * r * 0.25;
  float y = cosf((float)(0.25 * PI)) * r * 0.25;
  float center_x = w / 2.0;
  float center_y = h / 2.0;

  // Size of rendered string in pixels
  int px;
  int py;

  glPushMatrix();
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glColor3ub(0, 126, 29);  // same color as HDS
  glLineWidth(1.0);

  for (int i = 1; i <= 4; i++) {
    DrawArc(center_x, center_y, r * i * 0.25, 0.0, 2.0 * (float)PI, 360);
    const char *s = m_ri->GetDisplayRangeStr(i - 1);
    if (s) {
      m_FontNormal.RenderString(wxString::Format(wxT("%s"), s), center_x + x * (float)i, center_y + y * (float)i);
    }
  }

  if (m_pi->GetHeadingSource() != HEADING_NONE) {
    double heading;
    double predictor;
    switch (m_ri->GetOrientation()) {
      case ORIENTATION_HEAD_UP:
        heading = m_pi->GetHeadingTrue() + 180.;
        predictor = 180.;
        break;
      case ORIENTATION_STABILIZED_UP:
        heading = m_ri->m_course + 180.;
        predictor = m_pi->GetHeadingTrue() + 180. - m_ri->m_course;
        break;
      case ORIENTATION_NORTH_UP:
        heading = 180;
        predictor = m_pi->GetHeadingTrue() + 180;
        break;
      case ORIENTATION_COG_UP:
        heading = m_pi->GetCOG() + 180.;
        predictor = m_pi->GetHeadingTrue() + 180. - heading;
        break;
    }

    x = -sinf(deg2rad(predictor));
    y = cosf(deg2rad(predictor));
    glBegin(GL_LINE_STRIP);
    glVertex2f(center_x, center_y);
    glVertex2f(center_x + x * r * 2, center_y + y * r * 2);
    glEnd();

    for (int i = 0; i < 360; i += 5) {
      x = -sinf(deg2rad(i - heading)) * (r * 1.00 - 1);
      y = cosf(deg2rad(i - heading)) * (r * 1.00 - 1);

      wxString s;
      if (i % 90 == 0) {
        static char nesw[4] = {'N', 'E', 'S', 'W'};
        s = wxString::Format(wxT("%c"), nesw[i / 90]);
      } else if (i % 15 == 0) {
        s = wxString::Format(wxT("%u"), i);
      }

      m_FontNormal.GetTextExtent(s, &px, &py);
      if (x > 0) {
        x -= px;
      }
      if (y > 0) {
        y -= py;
      }
      m_FontNormal.RenderString(s, center_x + x, center_y + y);
    }
  }

  glPopAttrib();
  glPopMatrix();
}

void RadarCanvas::FillCursorTexture() {
#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 16

  // clang-format off
  const static char *cursor[CURSOR_HEIGHT] = {
    "................",
    "......*****.....",
    "......*---*.....",
    "......*---*.....",
    "......*---*.....",
    "..*****---*****.",
    "..*-----------*.",
    "..*-----------*.",
    "..*-----------*.",
    "..*****---*****.",
    "......*---*.....",
    "......*---*.....",
    "......*---*.....",
    "......*****.....",
    "................",
    "................",
  };
  // clang-format on

  GLubyte cursorTexture[CURSOR_HEIGHT][CURSOR_WIDTH][4];
  GLubyte *loc;
  int s, t;

  /* Setup RGB image for the texture. */
  loc = (GLubyte *)cursorTexture;
  for (t = 0; t < CURSOR_HEIGHT; t++) {
    for (s = 0; s < CURSOR_WIDTH; s++) {
      switch (cursor[t][s]) {
        case '*': /* White */
          loc[0] = 0xff;
          loc[1] = 0xff;
          loc[2] = 0xff;
          loc[3] = 0xff;
          break;
        case '-': /* black */
          loc[0] = 0x00;
          loc[1] = 0x00;
          loc[2] = 0x00;
          loc[3] = 0xff;
          break;
        default: /* transparent */
          loc[0] = 0x00;
          loc[1] = 0x00;
          loc[2] = 0x00;
          loc[3] = 0x00;
      }
      loc += 4;
    }
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, cursorTexture);
}

void RadarCanvas::RenderCursor(int w, int h) {
  double distance;
  double bearing;
  double radar_lat, radar_lon;

  int orientation = m_ri->GetOrientation();

  if (!isnan(m_ri->m_mouse_vrm)) {
    distance = m_ri->m_mouse_vrm * 1852.;
    bearing = m_ri->m_mouse_ebl[orientation];
  } else {
    if (isnan(m_ri->m_mouse_lat) || isnan(m_ri->m_mouse_lon) || !m_pi->GetRadarPosition(&radar_lat, &radar_lon)) {
      return;
    }
    // Can't compute this upfront, ownship may move...
    distance = local_distance(radar_lat, radar_lon, m_ri->m_mouse_lat, m_ri->m_mouse_lon) * 1852.;
    bearing = local_bearing(radar_lat, radar_lon, m_ri->m_mouse_lat, m_ri->m_mouse_lon);
    if (m_ri->GetOrientation() != ORIENTATION_NORTH_UP) {
      bearing -= m_pi->GetHeadingTrue();
    }
    // LOG_DIALOG(wxT("BR24radar_pi: Chart Mouse vrm=%f ebl=%f"), distance / 1852.0, bearing);
  }
  double full_range = wxMax(w, h) / 2.0;

  int display_range = m_ri->GetDisplayRange();
  double range = distance * full_range / display_range;

#define CURSOR_SCALE 1

  double center_x = w / 2.0;
  double center_y = h / 2.0;
  double angle = deg2rad(bearing);
  double x = center_x + sin(angle) * range - CURSOR_WIDTH * CURSOR_SCALE / 2;
  double y = center_y - cos(angle) * range - CURSOR_WIDTH * CURSOR_SCALE / 2;

  // LOG_DIALOG(wxT("BR24radar_pi: draw cursor angle=%.1f bearing=%.1f"), rad2deg(angle), bearing);

  if (!m_cursor_texture) {
    glGenTextures(1, &m_cursor_texture);
    glBindTexture(GL_TEXTURE_2D, m_cursor_texture);
    FillCursorTexture();
    LOG_DIALOG(wxT("BR24radar_pi: generated cursor texture # %u"), m_cursor_texture);
  }

  glColor3f(1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, m_cursor_texture);
  glBegin(GL_QUADS);
  glTexCoord2i(0, 0);
  glVertex2i(x, y);
  glTexCoord2i(1, 0);
  glVertex2i(x + CURSOR_SCALE * CURSOR_WIDTH, y);
  glTexCoord2i(1, 1);
  glVertex2i(x + CURSOR_SCALE * CURSOR_WIDTH, y + CURSOR_SCALE * CURSOR_HEIGHT);
  glTexCoord2i(0, 1);
  glVertex2i(x, y + CURSOR_SCALE * CURSOR_HEIGHT);
  glEnd();
}

void RadarCanvas::Render_EBL_VRM(int w, int h) {
  static const uint8_t rgb[BEARING_LINES][3] = {{22, 129, 154}, {45, 255, 254}};

  float full_range = wxMax(w, h) / 2.0;
  float center_x = w / 2.0;
  float center_y = h / 2.0;
  int display_range = m_ri->GetDisplayRange();
  int orientation = m_ri->GetOrientation();

  for (int b = 0; b < BEARING_LINES; b++) {
    float x, y;
    glColor3ubv(rgb[b]);
    glLineWidth(1.0);
    if (!isnan(m_ri->m_vrm[b])) {
      float scale = m_ri->m_vrm[b] * 1852.0 * full_range / display_range;
      if (m_ri->m_ebl[orientation][b] != nanl("")) {
        float angle = (float)deg2rad(m_ri->m_ebl[orientation][b]);
        x = center_x + sinf(angle) * full_range * 2.;
        y = center_y - cosf(angle) * full_range * 2.;
        glBegin(GL_LINES);
        glVertex2f(center_x, center_y);
        glVertex2f(x, y);
        glEnd();
      }
      DrawArc(center_x, center_y, scale, 0.f, 2.f * (float)PI, 360);
    }
  }
}

static void ResetGLViewPort(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);  // Next two operations on the project matrix stack
  glLoadIdentity();             // Reset projection matrix stack
  glOrtho(0, w, h, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);  // Reset matrick stack target back to GL_MODELVIEW
}

void RadarCanvas::Render(wxPaintEvent &evt) {
  int w, h;

  if (!IsShown() || !m_pi->IsInitialized()) {
    return;
  }

  GetClientSize(&w, &h);
  wxPaintDC(this);  // only to be used in paint events. use wxClientDC to paint
                    // outside the paint event

  if (!m_pi->IsOpenGLEnabled()) {
    return;
  }
  LOG_DIALOG(wxT("BR24radar_pi: %s render OpenGL canvas %d by %d "), m_ri->m_name.c_str(), w, h);

  SetCurrent(*m_context);

  glPushMatrix();
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  wxFont font = GetOCPNGUIScaledFont_PlugIn(_T("StatusBar"));
  m_FontNormal.Build(font);
  wxFont bigFont = GetOCPNGUIScaledFont_PlugIn(_T("Dialog"));
  bigFont.SetPointSize(bigFont.GetPointSize() + 2);
  bigFont.SetWeight(wxFONTWEIGHT_BOLD);
  m_FontBig.Build(bigFont);
  bigFont.SetPointSize(bigFont.GetPointSize() + 2);
  bigFont.SetWeight(wxFONTWEIGHT_NORMAL);
  m_FontMenu.Build(bigFont);
  bigFont.SetPointSize(bigFont.GetPointSize() + 10);
  bigFont.SetWeight(wxFONTWEIGHT_BOLD);
  m_FontMenuBold.Build(bigFont);

  wxColour bg = M_SETTINGS.ppi_background_colour;
  glClearColor(bg.Red() / 256.0, bg.Green() / 256.0, bg.Blue() / 256.0, bg.Alpha() / 256.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear the canvas
  glEnable(GL_TEXTURE_2D);                             // Enable textures
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // LAYER 1 - RANGE RINGS AND HEADINGS
  ResetGLViewPort(w, h);
  RenderRangeRingsAndHeading(w, h);

  PlugIn_ViewPort vp;

  if (m_pi->GetHeadingSource() != HEADING_NONE && m_pi->GetRadarPosition(&vp.clat, &vp.clon) &&
      M_SETTINGS.show_radar_target[m_ri->m_radar]) {
    // LAYER 2 - AIS AND ARPA TARGETS

    ResetGLViewPort(w, h);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    vp.m_projection_type = 4;  // Orthographic projection
    float full_range = wxMax(w, h) / 2.0;
    int display_range = m_ri->GetDisplayRange();

    switch (m_ri->GetOrientation()) {
      case ORIENTATION_HEAD_UP:
      case ORIENTATION_STABILIZED_UP:
        vp.rotation = deg2rad(-m_pi->GetHeadingTrue());
        break;
      case ORIENTATION_NORTH_UP:
        vp.rotation = 0.;
        break;
      case ORIENTATION_COG_UP:
        vp.rotation = deg2rad(-m_pi->GetCOG());
        break;
    }

    vp.view_scale_ppm = full_range / display_range;
    vp.skew = 0.;
    vp.pix_width = w;
    vp.pix_height = h;

    wxString aisTextFont = _("AIS Target Name");
    wxFont *aisFont = GetOCPNScaledFont_PlugIn(aisTextFont, 12);
    wxColour aisFontColor = GetFontColour_PlugIn(aisTextFont);

    if (aisFont) {
      wxColour newFontColor = M_SETTINGS.ais_text_colour;
      PlugInSetFontColor(aisTextFont, newFontColor);
      newFontColor = GetFontColour_PlugIn(aisTextFont);
    }
    PlugInAISDrawGL(this, vp);
    if (aisFont) {
      PlugInSetFontColor(aisTextFont, aisFontColor);
    }

    glPopAttrib();
    glPopMatrix();
  }

  // LAYER 3 - EBL & VRM

  ResetGLViewPort(w, h);
  Render_EBL_VRM(w, h);

  // LAYER 4 - RADAR RETURNS
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);  // Next two operations on the project matrix stack
  glLoadIdentity();             // Reset projection matrix stack
  if (w >= h) {
    glScaled(1.0, (float)-w / h, 1.0);
  } else {
    glScaled((float)h / w, -1.0, 1.0);
  }
  glMatrixMode(GL_MODELVIEW);  // Reset matrick stack target back to GL_MODELVIEW
  m_ri->RenderRadarImage(wxPoint(0, 0), 1.0, 0.0, false);

  // LAYER 5 - TEXTS & CURSOR
  ResetGLViewPort(w, h);
  RenderTexts(w, h);
  RenderCursor(w, h);

  glPopAttrib();
  glPopMatrix();
  glFlush();
  glFinish();
  SwapBuffers();

  wxGLContext *chart_context = m_pi->GetChartOpenGLContext();
  if (chart_context) {
    SetCurrent(*chart_context);
  } else {
    SetCurrent(*m_zero_context);  // Make sure OpenCPN -at least- doesn't overwrite our context info
  }
}

void RadarCanvas::OnMouseClick(wxMouseEvent &event) {
  int x, y, w, h;

  event.GetPosition(&x, &y);
  GetClientSize(&w, &h);

  int center_x = w / 2;
  int center_y = h / 2;

  //  LOG_DIALOG(wxT("BR24radar_pi: %s Mouse clicked at %d, %d"), m_ri->m_name.c_str(), x, y);
  if (x > 0 && x < w && y > 0 && y < h) {
    if (x >= w - m_menu_size.x && y < m_menu_size.y) {
      m_pi->ShowRadarControl(m_ri->m_radar, true);
    } else if ((x >= center_x - m_zoom_size.x / 2) && (x <= center_x + m_zoom_size.x / 2) &&
               (y > h - m_zoom_size.y + MENU_ROUNDING)) {
      if (x > center_x) {
        m_ri->AdjustRange(+1);
      } else {
        m_ri->AdjustRange(-1);
      }

    } else {
      double delta_x = x - center_x;
      double delta_y = y - center_y;

      double distance = sqrt(delta_x * delta_x + delta_y * delta_y);

      int display_range = m_ri->GetDisplayRange();

      double angle = fmod(rad2deg(atan2(delta_y, delta_x)) + 720. + 90., 360.0);

      double full_range = wxMax(w, h) / 2.0;

      double range = distance / (1852.0 * full_range / display_range);

      LOG_VERBOSE(wxT("BR24radar_pi: cursor in PPI at angle=%.1fdeg range=%.2fnm"), angle, range);
      m_ri->SetMouseVrmEbl(range, angle);
    }
  }
  event.Skip();
}

#define ZOOM_TIME 333       // 3 zooms per second
#define ZOOM_SENSITIVITY 0  // Increase to make less sensitive

void RadarCanvas::OnMouseWheel(wxMouseEvent &event) {
  // int delta = event.GetWheelDelta();
  int rotation = event.GetWheelRotation();

  wxLongLong now = wxGetUTCTimeMillis();

  //  LOG_INFO(wxT("BR24radar_pi: %s Mouse range wheel %d / %d"), m_ri->m_name.c_str(), rotation, delta);

  if (rotation) {
    if (m_pi->m_settings.reverse_zoom) {
      rotation = -rotation;
    }
    if (rotation > ZOOM_SENSITIVITY && m_last_mousewheel_zoom_in < now - ZOOM_TIME) {
      LOG_INFO(wxT("BR24radar_pi: %s Mouse zoom in"), m_ri->m_name.c_str());
      m_ri->AdjustRange(+1);
      m_last_mousewheel_zoom_in = now;
    } else if (rotation < -1 * ZOOM_SENSITIVITY && m_last_mousewheel_zoom_out < now - ZOOM_TIME) {
      LOG_INFO(wxT("BR24radar_pi: %s Mouse zoom out"), m_ri->m_name.c_str());
      m_ri->AdjustRange(-1);
      m_last_mousewheel_zoom_out = now;
    }
  }
}

PLUGIN_END_NAMESPACE
