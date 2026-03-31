/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *                                                                         *
 *   This m_program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This m_program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this m_program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#include "RadarDrawShader.h"

#include "RadarInfo.h"
#include "drawutil.h"
#include "shaderutil.h"

PLUGIN_BEGIN_NAMESPACE

// identity vertex program (does nothing special)
static const char *VertexShaderText =
    "void main() \n"
    "{ \n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
    "   gl_Position = ftransform(); \n"
    "} \n";

// Convert to rectangular to polar coordinates for radar image in texture
// No longer used, always use FragmentShaderColorText so we can draw trails.
#ifdef NEVER
static const char *FragmentShaderText =
    "uniform sampler2D tex2d; \n"
    "void main() \n"
    "{ \n"
    "   float d = length(gl_TexCoord[0].xy);\n"
    "   if (d >= 1.0) \n"
    "      discard; \n"
    "   float a = atan(gl_TexCoord[0].y, gl_TexCoord[0].x) / 6.28318; \n"
    "   gl_FragColor = vec4(1, 0, 0, texture2D(tex2d, vec2(d, a)).x); \n"
    "} \n";
#endif

static const char *FragmentShaderColorText =
    "uniform sampler2D tex2d; \n"
    "void main() \n"
    "{ \n"
    "   float d = length(gl_TexCoord[0].xy);\n"
    "   if (d >= 1.0) \n"
    "      discard; \n"
    "   float a = atan(gl_TexCoord[0].y, gl_TexCoord[0].x) / 6.28318; \n"
    "   gl_FragColor = texture2D(tex2d, vec2(d, a)); \n"
    "} \n";

bool RadarDrawShader::Init(size_t spokes, size_t spoke_len_max) {
  wxCriticalSectionLocker lock(m_exclusive);

  m_format = GL_RGBA;
  m_channels = SHADER_COLOR_CHANNELS;
  m_spokes = spokes;
  m_spoke_len_max = spoke_len_max;

  if (!CompileShader && !ShadersSupported()) {
    wxLogError(wxT("the OpenGL system of this computer does not support shader m_programs"));
    return false;
  }

  Reset();

  if (!CompileShaderText(&m_vertex, GL_VERTEX_SHADER, VertexShaderText) ||
      !CompileShaderText(&m_fragment, GL_FRAGMENT_SHADER, FragmentShaderColorText)) {
    wxLogError(wxT("the OpenGL system of this computer failed to compile shader programs"));
    return false;
  }

  m_program = LinkShaders(m_vertex, m_fragment);
  if (m_program == 0) {
    wxLogError(wxT("GPU oriented OpenGL failed to link shader program"));
    return false;
  }

  for (int i = 0; i <= MAX_CHART_CANVAS; i++) {
    glGenTextures(1, &m_texture[i]);
    glBindTexture(GL_TEXTURE_2D, m_texture[i]);

    if (m_data[i]) {
      free(m_data[i]);
    }
    m_data[i] = (unsigned char *)calloc(SHADER_COLOR_CHANNELS, m_spoke_len_max * m_spokes);
  
  // Tell the GPU the size of the texture:
  glTexImage2D(/* target          = */ GL_TEXTURE_2D,
               /* level           = */ 0,
               /* internal_format = */ m_format,
               /* width           = */ m_spoke_len_max,
               /* heigth          = */ m_spokes,
               /* border          = */ 0,
               /* format          = */ m_format,
               /* type            = */ GL_UNSIGNED_BYTE,
               /* data            = */ m_data[i]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_start_line[i] = -1;
    m_lines[i] = 0;
  }
  return true;
}

void RadarDrawShader::Reset() {
  if (m_vertex) {
    DeleteShader(m_vertex);
    m_vertex = 0;
  }
  if (m_fragment) {
    DeleteShader(m_fragment);
    m_fragment = 0;
  }
  if (m_program) {
    DeleteProgram(m_program);
    m_program = 0;
  }
  
  for (int canvas = 0; canvas <= MAX_CHART_CANVAS; canvas++) {
    if (m_data[canvas]) {
      free(m_data[canvas]);
      m_data[canvas] = 0;
    }
    if (m_texture[canvas]) {
      glDeleteTextures(1, &m_texture[canvas]);
      m_texture[canvas] = 0;
    }
  }
}

RadarDrawShader::~RadarDrawShader() {
  wxCriticalSectionLocker lock(m_exclusive);

  Reset();
}

void RadarDrawShader::DrawRadarOverlayImage(int canv,double radar_scale, double panel_rotate) {
  wxCriticalSectionLocker lock(m_exclusive);

  int canvas = canv;
  int start_radius = m_ri->m_start_overlay_r[canv];
  if (start_radius == 0) {
    // this radar starts at 0, there is no second vertex array
    canvas = m_ri->m_pi->m_max_canvas;  // here a full image is stored
  }

  if (!m_program || !m_texture[canvas] || !m_data[canvas]) {
    return;
  }

  glPushAttrib(GL_TEXTURE_BIT);

  UseProgram(m_program);

  glBindTexture(GL_TEXTURE_2D, m_texture[canvas]);

  if (m_start_line[canvas] > -1) {
    // Since the last time we have received data from [m_start_line, m_end_line>
    // so we only need to update the texture for those data lines.
    if (m_start_line[canvas] + m_lines[canvas] > (int)m_spokes) {
      int end_line = (m_start_line[canvas] + m_lines[canvas]) % m_spokes;
      // if the new data partly wraps past the end of the texture
      // tell it the two parts separately
      // First remap [0, m_end_line>
      glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                      /* level =    */ 0,
                      /* x-offset = */ 0,
                      /* y-offset = */ 0,
                      /* width =    */ m_spoke_len_max,
                      /* height =   */ end_line,
                      /* format =   */ m_format,
                      /* type =     */ GL_UNSIGNED_BYTE,
                      /* pixels =   */ m_data[canvas]);
      // And then remap [m_start_line, m_spokes>
      glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                      /* level =    */ 0,
                      /* x-offset = */ 0,
                      /* y-offset = */ m_start_line[canvas],
                      /* width =    */ m_spoke_len_max,
                      /* height =   */ m_spokes - m_start_line[canvas],
                      /* format =   */ m_format,
                      /* type =     */ GL_UNSIGNED_BYTE,
                      /* pixels =   */ m_data[canvas] + m_start_line[canvas] * m_spoke_len_max * m_channels);
    } else {
      // Map [m_start_line, m_end_line>
      glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                      /* level =    */ 0,
                      /* x-offset = */ 0,
                      /* y-offset = */ m_start_line[canvas],
                      /* width =    */ m_spoke_len_max,
                      /* height =   */ m_lines[canvas],
                      /* format =   */ m_format,
                      /* type =     */ GL_UNSIGNED_BYTE,
                      /* pixels =   */ m_data[canvas] + m_start_line[canvas] * m_spoke_len_max * m_channels);
    }
    m_start_line[canvas] = -1;
    m_lines[canvas] = 0;
  }

  // We tell the GPU to draw a square from (-512,-512) to (+512,+512).
  // The shader morphs this into a circle.
  float fullscale = m_spoke_len_max;
  glBegin(GL_QUADS);
  glTexCoord2f(-1, -1);
  glVertex2f(-fullscale, -fullscale);
  glTexCoord2f(1, -1);
  glVertex2f(fullscale, -fullscale);
  glTexCoord2f(1, 1);
  glVertex2f(fullscale, fullscale);
  glTexCoord2f(-1, 1);
  glVertex2f(-fullscale, fullscale);
  glEnd();

  UseProgram(0);
  glPopAttrib();
}

void RadarDrawShader::DrawRadarPanelImage(double panel_scale, double panel_rotate) { 
  DrawRadarOverlayImage(m_ri->m_pi->m_max_canvas, 1., 0.);
} 

void RadarDrawShader::ProcessRadarSpoke(int transparency, SpokeBearing angle, uint8_t *data, size_t len, GeoPosition spoke_pos,
                                        bool overlay) {
  GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - transparency) / MAX_OVERLAY_TRANSPARENCY;
  wxCriticalSectionLocker lock(m_exclusive);
  for (int canv = 0; canv <= m_ri->m_pi->m_max_canvas; canv++) {
    int canvas = canv;

    size_t start_r = 0;
    {
      wxCriticalSectionLocker lock(m_ri->m_pi->m_sort_tx_radars);
      if (canvas == m_ri->m_pi->m_max_canvas) {
        start_r = 0;  // make one full image for panel or overlay
      } else {
        start_r = m_ri->m_start_overlay_r[canvas];
        if (start_r == 0) {
          continue;  // no need to make a second full imagem, this is done at canvas == m_pi->m_max_canvas
        }
      }
    }

    if (m_start_line[canvas] == -1) {
      m_start_line[canvas] = angle;  // Note that this only runs once after each draw,
    }
    if (m_lines[canvas] < (int)m_spokes) {
      m_lines[canvas]++;
    }

    if (m_channels == SHADER_COLOR_CHANNELS) {
      unsigned char *d = m_data[canvas] + (angle * m_spoke_len_max) * m_channels;
      for (size_t r = 0; r < start_r; r++) {
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
      }
      for (size_t r = start_r; r < len; r++) {
        GLubyte strength = data[r];
        BlobColour colour = m_ri->m_colour_map[strength];
        d[0] = m_ri->m_colour_map_rgb[colour].Red();
        d[1] = m_ri->m_colour_map_rgb[colour].Green();
        d[2] = m_ri->m_colour_map_rgb[colour].Blue();
        d[3] = colour != BLOB_NONE ? alpha : 0;
        d += m_channels;
      }
      for (size_t r = len; r < m_spoke_len_max; r++) {
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
      }
    } else {
      unsigned char *d = m_data[canvas] + (angle * m_spoke_len_max) + start_r;
      for (size_t r = start_r; r < len; r++) {
        GLubyte strength = data[r];
        BlobColour colour = m_ri->m_colour_map[strength];
        *d++ = (m_ri->m_colour_map_rgb[colour].Red() * alpha) >> 8;
      }
      for (size_t r = len; r < m_spoke_len_max; r++) {
        *d++ = 0;
      }
    }
  }
}

PLUGIN_END_NAMESPACE
