/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
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
#include "RadarInfo.h"
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

RadarCanvas::RadarCanvas(radar_pi *pi, RadarInfo *ri, wxWindow *parent, wxSize size)
    : wxGLCanvas(parent, wxID_ANY, attribs, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE | wxBG_STYLE_CUSTOM, _T("")) {
  m_parent = parent;
  m_pi = pi;
  m_ri = ri;
  m_context = new wxGLContext(this);
  m_zero_context = new wxGLContext(this);
  m_cursor_texture = 0;
  m_last_mousewheel_zoom_in = 0;
  m_last_mousewheel_zoom_out = 0;

  LOG_VERBOSE(wxT("radar_pi: %s create OpenGL canvas"), m_ri->m_name.c_str());
  Refresh(false);
}

RadarCanvas::~RadarCanvas() {
  LOG_VERBOSE(wxT("radar_pi: %s destroy OpenGL canvas"), m_ri->m_name.c_str());
  delete m_context;
  delete m_zero_context;
  if (m_cursor_texture) {
    glDeleteTextures(1, &m_cursor_texture);
    m_cursor_texture = 0;
  }
}

void RadarCanvas::OnSize(wxSizeEvent &evt) {
  wxSize parentSize = m_parent->GetSize();
  LOG_DIALOG(wxT("radar_pi: %s resize OpenGL canvas to %d, %d"), m_ri->m_name.c_str(), parentSize.x, parentSize.y);
  Refresh(false);
  if (GetSize() != parentSize) {
    SetSize(parentSize);
  }
}

void RadarCanvas::OnMove(wxMoveEvent &evt) {
  wxPoint pos = m_parent->GetPosition();
  LOG_DIALOG(wxT("radar_pi: %s move OpenGL canvas to %d, %d"), m_ri->m_name.c_str(), pos.x, pos.y);
}

void RadarCanvas::RenderTexts(int w, int h) {
  int x, y;
  int menu_x;
  wxString s;
  RadarState state = (RadarState)m_ri->m_state.GetValue();

#define MENU_ROUNDING 4
#define MENU_BORDER 8
#define MENU_EXTRA_WIDTH 32

  // Draw Menu in the top right

  s = _("Menu");
  m_FontMenu.GetTextExtent(s, &x, &y);
  menu_x = x;

  // Calculate the size of the rounded rect, this is also where you can 'click'...
  m_menu_size.x = x + 2 * (MENU_BORDER + MENU_EXTRA_WIDTH);
  m_menu_size.y = y + 2 * (MENU_BORDER);

  if (state != RADAR_OFF) {
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
  }

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

  if (state != RADAR_OFF) {
    wxSize i;
    i.x = w - 5 - menu_x / 2;
    i.y = h - 5;
    i = RenderControlItem(i, m_ri->m_rain, CT_RAIN, _("Rain"));
    i.y -= 5;
    i = RenderControlItem(i, m_ri->m_sea, CT_SEA, _("Sea"));
    i.y -= 5;
    i = RenderControlItem(i, m_ri->m_gain, CT_GAIN, _("Gain"));
  }
}

/*
 * Receives bottom mid part of canvas to draw, returns back top mid
 */
wxSize RadarCanvas::RenderControlItem(wxSize loc, RadarControlItem &item, ControlType ct, wxString name) {
  if (!m_ri->m_control_dialog || m_ri->m_control_dialog->m_ctrl[ct].type == CT_NONE) {
    return loc;
  }

  ControlInfo ci = m_ri->m_control_dialog->m_ctrl[ct];
  int tx, ty;
  int state = item.GetState();
  int value = item.GetValue();
  wxString label;

  switch (item.GetState()) {
    case RCS_OFF:
      glColor4ub(100, 100, 100, 255);  // Grey
      label << _("Off");
      value = -1;
      break;

    case RCS_MANUAL:
      glColor4ub(255, 100, 100, 255);  // Reddish
      label.Printf(wxT("%d"), value);
      break;

    default:
      glColor4ub(200, 255, 200, 255);  // Greenish
      if (ci.autoNames && state > RCS_MANUAL && state <= RCS_MANUAL + ci.autoValues) {
        label
            << ci.autoNames[state - RCS_AUTO_1];  // A little shorter than in the control, but here we have colour to indicate Auto.
      } else {
        label << _("Auto");
      }
      if (!m_ri->m_showManualValueInAuto) {
        value = -1;
      }
      break;
  }

  m_FontNormal.GetTextExtent(label, &tx, &ty);
  loc.y -= ty;
  m_FontNormal.RenderString(label, loc.x - tx / 2, loc.y);

  m_FontNormal.GetTextExtent(name, &tx, &ty);
  loc.y -= ty;
  m_FontNormal.RenderString(name, loc.x - tx / 2, loc.y);

  // Draw a semi circle, 270 degrees when 100%
  if (value > 0) {
    glLineWidth(2.0);
    DrawArc(loc.x, loc.y + ty, ty + 3, (float)deg2rad(-225), (float)deg2rad(value * 270. / ci.maxValue), value / 2);
  }
  return loc;
}

void RadarCanvas::RenderRangeRingsAndHeading(int w, int h, float r) {
  // Max range ringe
  // Size of rendered string in pixels
  glPushMatrix();
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  double offset = r / 2.;  // half of the radar_radius
  double heading = 180.;
  if (m_pi->GetHeadingSource() != HEADING_NONE) {
    switch (m_ri->GetOrientation()) {
    case ORIENTATION_HEAD_UP:
      heading += 0.;
      m_ri->m_predictor = 0.;    // predictor in the direction of the line on the radar window
      break;
    case ORIENTATION_STABILIZED_UP:
      heading += m_ri->m_course;
      m_ri->m_predictor = m_pi->GetHeadingTrue() - m_ri->m_course;
      break;
    case ORIENTATION_NORTH_UP:
      m_ri->m_predictor = m_pi->GetHeadingTrue();
      break;
    case ORIENTATION_COG_UP:
      heading += m_pi->GetCOG();
      m_ri->m_predictor = m_pi->GetHeadingTrue() - heading - 180.;
      break;
    }
  }
  else {
    m_ri->m_predictor = 0.;
  }
// set off center offset in the direction of the predictor
  if (m_ri->m_view_center.GetValue()) {
    glRotated(m_ri->m_predictor, 0., 0., 1.);
    if (m_ri->m_view_center.GetValue() == FORWARD_VIEW) {
      glTranslated(0., offset, 0.);  // shift range rings and predictor
    }
    else {
      glTranslated(0., -offset, 0.);
    }
    glRotated(-m_ri->m_predictor, 0., 0., 1.);
  }

  glColor3ub(0, 126, 29);  // same color as HDS
  glLineWidth(1.0);

  int meters = m_ri->m_range.GetValue();
  int rings = 1;

  if (meters > 0) {
    // Instead of computing various modulo we just check which ranges
    // result in a non-empty range string.
    // We try 3/4th, 2/3rd, 1/2, falling back to 1 ring = no subrings

    for (rings = 4; rings > 1; rings--) {
      wxString s = m_ri->GetDisplayRangeStr(meters * (rings - 1) / rings, false);
      if (s.length() > 0) {
        break;
      }
    }
  }

  float x = sinf((float)(0.25 * PI)) * r / (double)rings;
  float y = cosf((float)(0.25 * PI)) * r / (double)rings;
  // Position of the range texts
  float center_x = w / 2.0;
  float center_y = h / 2.0;
  int px;
  int py;

  for (int i = 1; i <= rings; i++) {
    DrawArc(center_x, center_y, r * i / (double)rings, 0.0, 2.0 * (float)PI, 360);
    if (meters != 0) {
      wxString s = m_ri->GetDisplayRangeStr(meters * i / rings, false);
      if (s.length() > 0) {
        m_FontNormal.RenderString(s, center_x + x * i, center_y + y * i);
      }
    }
  }

  //if (m_pi->GetHeadingSource() != HEADING_NONE) {

    x = sinf((float)deg2rad(m_ri->m_predictor));
    y = -cosf((float)deg2rad(m_ri->m_predictor));
    glLineWidth(1.0);

    glBegin(GL_LINES);
    glVertex2f(center_x, center_y);
    glVertex2f(center_x + x * r * 2, center_y + y * r * 2);

    for (int i = 0; i < 360; i += 10) {
      x = -sinf(deg2rad(i - heading)) * (r * 1.00);
      y = cosf(deg2rad(i - heading)) * (r * 1.00);

      // draw a little 'tick' outward from the outermost range circle (which is already drawn)
      glVertex2f(center_x + x, center_y + y);
      glVertex2f(center_x + x * 1.02, center_y + y * 1.02);
    }
    glEnd();
    for (int i = 0; i < 360; i += 30) {
      x = -sinf(deg2rad(i - heading)) * (r * 1.00 - 1);
      y = cosf(deg2rad(i - heading)) * (r * 1.00 - 1);

      wxString s;
      if (i % 90 == 0 && (m_pi->GetHeadingSource() != HEADING_NONE)) {
        static char nesw[4] = {'N', 'E', 'S', 'W'};
        s = wxString::Format(wxT("%c"), nesw[i / 90]);
      } else {
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
  //}

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

void RadarCanvas::RenderCursor(int w, int h, float radius) {
  double distance;
  double bearing;
  GeoPosition pos;

  int orientation = m_ri->GetOrientation();

  if (!isnan(m_ri->m_mouse_vrm)) {
    distance = m_ri->m_mouse_vrm * 1852.;
    bearing = m_ri->m_mouse_ebl[orientation];
  } else {
    if (isnan(m_ri->m_mouse_pos.lat) || isnan(m_ri->m_mouse_pos.lon) || !m_ri->GetRadarPosition(&pos)) {
      return;
    }
    // Can't compute this upfront, ownship may move...
    distance = local_distance(pos, m_ri->m_mouse_pos) * 1852.;
    bearing = local_bearing(pos, m_ri->m_mouse_pos);
    if (m_ri->GetOrientation() != ORIENTATION_NORTH_UP) {
      bearing -= m_pi->GetHeadingTrue();
    }
    // LOG_DIALOG(wxT("radar_pi: Chart Mouse vrm=%f ebl=%f"), distance / 1852.0, bearing);
  }

  int display_range = m_ri->GetDisplayRange();
  double range = distance * radius / display_range;

#define CURSOR_SCALE 1

  double center_x = w / 2.0;
  double center_y = h / 2.0;
  double angle = deg2rad(bearing);
  double x = center_x + sin(angle) * range - CURSOR_WIDTH * CURSOR_SCALE / 2;
  double y = center_y - cos(angle) * range - CURSOR_WIDTH * CURSOR_SCALE / 2;


  if (!m_cursor_texture) {
    glGenTextures(1, &m_cursor_texture);
    glBindTexture(GL_TEXTURE_2D, m_cursor_texture);
    FillCursorTexture();
    LOG_DIALOG(wxT("radar_pi: generated cursor texture # %u"), m_cursor_texture);
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

void RadarCanvas::Render_EBL_VRM(int w, int h, float radius) {
  static const uint8_t rgb[BEARING_LINES][3] = {{22, 129, 154}, {45, 255, 254}};

  float center_x = w / 2.0;
  float center_y = h / 2.0;
  int display_range = m_ri->GetDisplayRange();
  int orientation = m_ri->GetOrientation();

  glPushMatrix();
  if (m_ri->m_view_center.GetValue()) {
    double offset = (double)wxMax(w, h) * CHART_SCALE_OFFSET / 4.;
    glRotated(m_ri->m_predictor, 0., 0., 1.);
    if (m_ri->m_view_center.GetValue() == FORWARD_VIEW) {
      glTranslated(0., offset, 0.);
    }
    else {                                 // aft view
      glTranslated(0., -offset, 0.);
    }
    glRotated(-m_ri->m_predictor, 0., 0., 1.);
  }


  for (int b = 0; b < BEARING_LINES; b++) {
    float x, y;
    glColor3ubv(rgb[b]);
    glLineWidth(1.0);
    if (!isnan(m_ri->m_vrm[b])) {
      float scale = m_ri->m_vrm[b] * 1852.0 * radius / display_range;
      if (m_ri->m_ebl[orientation][b] != nanl("")) {
        float angle = (float)deg2rad(m_ri->m_ebl[orientation][b]);
        x = center_x + sinf(angle) * radius * 2.;
        y = center_y - cosf(angle) * radius * 2.;
        glBegin(GL_LINES);
        glVertex2f(center_x, center_y);
        glVertex2f(x, y);
        glEnd();
      }
      DrawArc(center_x, center_y, scale, 0.f, 2.f * (float)PI, 360);
    }
  }
  glPopMatrix();
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
  LOG_VERBOSE(wxT("radar_pi: %s render OpenGL canvas %d by %d "), m_ri->m_name.c_str(), w, h);
  double chart_scale;
  if (m_ri->m_view_center.GetValue()) {
    chart_scale = CHART_SCALE_OFFSET;
  }
  else {
    chart_scale = CHART_SCALE_CENTER;
  }

  float radar_radius = (float)wxMax(w, h) * (float)chart_scale / 2.0;

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
  RenderRangeRingsAndHeading(w, h, radar_radius);

  PlugIn_ViewPort vp;
  GeoPosition pos;

  if (m_pi->GetHeadingSource() != HEADING_NONE && m_ri->GetRadarPosition(&pos) && m_ri->m_target_on_ppi.GetValue() > 0) {
    // LAYER 2 - AIS AND ARPA TARGETS

    ResetGLViewPort(w, h);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    
    if (m_ri->m_view_center.GetValue()) {
      double offset = (double)wxMax(w, h) * CHART_SCALE_OFFSET / 4.;
      glRotated(m_ri->m_predictor, 0., 0., 1.);
      if (m_ri->m_view_center.GetValue() == FORWARD_VIEW) {
        glTranslated(0., offset, 0.);
      }
      else {                                 // aft view
        glTranslated(0., -offset, 0.);
      }
      glRotated(-m_ri->m_predictor, 0., 0., 1.);
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    vp.m_projection_type = 4;  // Orthographic projection
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

    vp.view_scale_ppm = radar_radius / display_range;
    vp.skew = 0.;
    vp.pix_width = w;
    vp.pix_height = h;
    vp.clat = pos.lat;
    vp.clon = pos.lon;

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
  Render_EBL_VRM(w, h, radar_radius);

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

  m_ri->RenderRadarImage1(wxPoint(0, 0), chart_scale / m_ri->m_range.GetValue(), 0.0, false);

  // LAYER 5 - TEXTS & CURSOR
  ResetGLViewPort(w, h);
  RenderTexts(w, h);
  glPushMatrix();
  double offset = (double)wxMax(w, h) * chart_scale / 4.;
  if (m_ri->m_view_center.GetValue()) {
    glRotated(m_ri->m_predictor, 0., 0., 1.);
    if (m_ri->m_view_center.GetValue() == FORWARD_VIEW) {
      glTranslated(0., offset, 0.);       // shift mouse pointer
    }
    else {                                 // aft view
      glTranslated(0., -offset, 0.);
    }
    glRotated(-m_ri->m_predictor, 0., 0., 1.);
  }
  RenderCursor(w, h, radar_radius);
  glPopMatrix();

  glPopAttrib();
  glPopMatrix();
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

  double chart_scale;
  if (m_ri->m_view_center.GetValue()) {
    chart_scale = CHART_SCALE_OFFSET;
  }
  else {
    chart_scale = CHART_SCALE_CENTER;
  }
  double offset = (double)wxMax(w, h) * chart_scale / 4.;  // half of the radar_radius
  if (m_ri->m_view_center.GetValue()) {
    
    if (m_ri->m_view_center.GetValue() == 1) {
      center_x -= offset * sin(deg2rad(m_ri->m_predictor));  // horizontal
      center_y += offset * cos(deg2rad(m_ri->m_predictor));
    }
    else {                                                 // look aft
      center_x += int (offset * sin(deg2rad(m_ri->m_predictor)));
      center_y -= int (offset * cos(deg2rad(m_ri->m_predictor)));
    }
  }
    LOG_DIALOG(wxT("radar_pi: %s Mouse clicked at %d, %d"), m_ri->m_name.c_str(), x, y);
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

      double full_range = chart_scale * wxMax(w, h) / 2.0;

      double range = distance / (1852.0 * full_range / display_range);

      LOG_VERBOSE(wxT("radar_pi: cursor in PPI at angle=%.1fdeg range=%.2fnm"), angle, range);
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

  //  LOG_INFO(wxT("radar_pi: %s Mouse range wheel %d / %d"), m_ri->m_name.c_str(), rotation, delta);

  if (rotation) {
    if (m_pi->m_settings.reverse_zoom) {
      rotation = -rotation;
    }
    if (rotation > ZOOM_SENSITIVITY && m_last_mousewheel_zoom_in < now - ZOOM_TIME) {
      LOG_INFO(wxT("radar_pi: %s Mouse zoom in"), m_ri->m_name.c_str());
      m_ri->AdjustRange(+1);
      m_last_mousewheel_zoom_in = now;
    } else if (rotation < -1 * ZOOM_SENSITIVITY && m_last_mousewheel_zoom_out < now - ZOOM_TIME) {
      LOG_INFO(wxT("radar_pi: %s Mouse zoom out"), m_ri->m_name.c_str());
      m_ri->AdjustRange(-1);
      m_last_mousewheel_zoom_out = now;
    }
  }
}

PLUGIN_END_NAMESPACE
