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

#include "br24radar_pi.h"

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

bool br24Shader::Init( br24radar_pi * ppi, int newColorOption )
{
    pPlugin = ppi;
    colorOption = newColorOption;

    if (!CompileShader && !ShadersSupported()) {
        wxLogMessage(wxT("BR24radar_pi: the OpenGL system of this computer does not support shader programs"));
        return false;
    }

    if (!CompileShaderText(&vertex, GL_VERTEX_SHADER, VertexShaderText)
     || !CompileShaderText(&fragment, GL_FRAGMENT_SHADER, colorOption > 0 ? FragmentShaderColorText : FragmentShaderText)) {
        return false;
    }

    program = LinkShaders(vertex, fragment);
    if (!program) {
        return false;
    }

    if (!texture) {
        glGenTextures(1, &texture);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, colorOption > 0 ? GL_RGBA : GL_LUMINANCE,
                 RETURNS_PER_LINE, LINES_PER_ROTATION, 0,
                 colorOption > 0 ? GL_RGBA : GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    wxLogMessage(wxT("BR24radar_pi: GPU oriented OpenGL shader for %d colours loaded"), (colorOption > 0) ? 4 : 1);
    start_line = LINES_PER_ROTATION;
    end_line = 0;

    return true;
}

void br24Shader::DrawRadarImage( wxPoint center, double heading, double scale, bool overlay )
{
    if (start_line == LINES_PER_ROTATION) {
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
    glRotatef(heading + 270, 0, 0, 1); // OpenGL rotation is different to North = 0 = Up.

    UseProgram(program);

    glBindTexture(GL_TEXTURE_2D, texture);

    int format, channels;
    if (colorOption)
        format = GL_RGBA, channels = 4;
    else
        format = GL_LUMINANCE, channels = 1;

    // Set texture to the returned radar returns
    if (end_line < start_line) {
        // if the new data wraps past the end of the texture
        // tell it the two parts separately
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ 0
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ end_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ data
                       );
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ start_line
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ LINES_PER_ROTATION - start_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ data + start_line * RETURNS_PER_LINE * channels
                       );
    } else {
        glTexSubImage2D( /* target =   */ GL_TEXTURE_2D
                       , /* level =    */ 0
                       , /* x-offset = */ 0
                       , /* y-offset = */ start_line
                       , /* width =    */ RETURNS_PER_LINE
                       , /* height =   */ end_line - start_line
                       , /* format =   */ format
                       , /* type =     */ GL_UNSIGNED_BYTE
                       , /* pixels =   */ data + start_line * RETURNS_PER_LINE * channels
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
    if (pPlugin->settings.verbose >= 2) {
        wxLogMessage(wxT("BR24radar_pi: used shader %d line %d-%d"), program, start_line, end_line);
    }
    // start_line = -1;
    // end_line = 0;

    return;
}

void br24Shader::ClearSpoke( int angle )
{
    if (!data) // buffers not yet allocated for us, should not happen
        return;

    if (start_line > angle) {
        start_line = angle;
    }
    if (end_line < angle + 1) {
      end_line = angle + 1;
    }

    // zero out texture data
    if (colorOption)
        memset(data + 4*angle*RETURNS_PER_LINE, 0, 4*RETURNS_PER_LINE);
    else
        memset(data + angle*RETURNS_PER_LINE, 0, RETURNS_PER_LINE);
}

void br24Shader::SetBlob( int angle_begin, int angle_end, int r_begin, int r_end, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
    for (int arc = angle_begin; arc < angle_end; arc++) {
        int angle = MOD_ROTATION2048(arc);
        if (colorOption) {
            unsigned char *d = data + (angle * RETURNS_PER_LINE + r_begin) * 4;
            for (int j = r_begin; j < r_end; j++) {
                for (int k = 0; k < 4; k++) {
                    d[0] = red;
                    d[1] = green;
                    d[2] = blue;
                    d[3] = alpha;
                }
                d += 4;
            }
        } else {
            memset(data + angle * RETURNS_PER_LINE + r_begin, ((int)red * alpha) >> 8, r_end - r_begin);
        }
    }
}
