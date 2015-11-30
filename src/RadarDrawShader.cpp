/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Navico BR24 Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2013 by Kees Verruijt         canboat@verruijt.net *
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

// identity vertex program (does nothing special)
static const char *VertexShaderText =
   "void main() \n"
   "{ \n"
   "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
   "   gl_Position = ftransform(); \n"
   "} \n";

// Convert to rectangular to polar coordinates for radar image in texture
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

bool RadarDrawShader::Init( int color_option )
{
    m_color_option = color_option;

    if (!CompileShader && !ShadersSupported()) {
        wxLogMessage(wxT("BR24radar_pi: the OpenGL system of this computer does not support shader m_programs"));
        return false;
    }

    if (!CompileShaderText(&m_vertex, GL_VERTEX_SHADER, VertexShaderText)
     || !CompileShaderText(&m_fragment, GL_FRAGMENT_SHADER, m_color_option > 0 ? FragmentShaderColorText : FragmentShaderText)) {
        return false;
    }

    m_program = LinkShaders(m_vertex, m_fragment);
    if (!m_program) {
        return false;
    }

    if (!m_texture) {
        glGenTextures(1, &m_texture);
    }
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, m_color_option > 0 ? GL_RGBA : GL_LUMINANCE,
                 RETURNS_PER_LINE, LINES_PER_ROTATION, 0,
                 m_color_option > 0 ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, m_data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    wxLogMessage(wxT("BR24radar_pi: GPU oriented OpenGL shader for %d colours loaded"), (m_color_option > 0) ? 4 : 1);
    m_start_line = LINES_PER_ROTATION;
    m_end_line = 0;

    return true;
}

void RadarDrawShader::DrawRadarImage( wxPoint center, double scale, double rotation, bool overlay )
{
    if (m_start_line == LINES_PER_ROTATION || !m_program) {
        return;
    }

    glPushMatrix();
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_HINT_BIT);

    glTranslated(center.x, center.y, 0);
    glScaled(scale, scale, 1.);

    if (overlay) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    UseProgram(m_program);

    glBindTexture(GL_TEXTURE_2D, m_texture);

    int format, channels;
    if (m_color_option)
        format = GL_RGBA, channels = 4;
    else
        format = GL_LUMINANCE, channels = 1;

    // Set texture to the returned radar returns
    if (m_end_line < m_start_line) {
        // if the new data wraps past the end of the texture
        // tell it the two parts separately
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ 0
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ m_end_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ m_data
                       );
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ m_start_line
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ LINES_PER_ROTATION - m_start_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ m_data + m_start_line * RETURNS_PER_LINE * channels
                       );
    } else {
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ m_start_line
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ m_end_line - m_start_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ m_data + m_start_line * RETURNS_PER_LINE * channels
                       );
    }

    // We tell the GPU to draw a square from (-512,-512) to (+512,+512).
    // The shader morphs this into a circle.
    float fullscale = 512;
    glBegin(GL_QUADS);
    glTexCoord2f(-1, -1);  glVertex2f(-fullscale, -fullscale);
    glTexCoord2f( 1, -1);  glVertex2f( fullscale, -fullscale);
    glTexCoord2f( 1,  1);  glVertex2f( fullscale,  fullscale);
    glTexCoord2f(-1,  1);  glVertex2f(-fullscale,  fullscale);
    glEnd();

    UseProgram(0);
    glPopAttrib();
    glPopMatrix();
    if (m_pi->m_settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: used shader %d line %d-%d"), m_program, m_start_line, m_end_line);
    }
    // m_start_line = -1;
    // m_end_line = 0;

    return;
}

void RadarDrawShader::ProcessRadarSpoke( SpokeBearing angle, UINT8 * data, size_t len )
{
    GLubyte alpha = 255 * (MAX_OVERLAY_TRANSPARENCY - m_pi->m_settings.overlay_transparency) / MAX_OVERLAY_TRANSPARENCY;

    if (m_color_option) {
        unsigned char * d = m_data + (angle * RETURNS_PER_LINE) * 4;
        for (size_t r = 0; r < len; r++) {
            GLubyte strength = data[r];
            d[0] = m_pi->m_color_map_red[strength];
            d[1] = m_pi->m_color_map_green[strength];
            d[2] = m_pi->m_color_map_blue[strength];
            d[3] = alpha;
            d += 4;
        }
    } else {
        unsigned char * d = m_data + (angle * RETURNS_PER_LINE);
        for (size_t r = 0; r < len; r++) {
            GLubyte strength = data[r];
            *d++ = (m_pi->m_color_map_red[strength] * alpha) >> 8;
        }
    }
}

// vim: sw=4:ts=8:
