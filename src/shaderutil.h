#ifndef _SHADER_UTIL_H_
#define _SHADER_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __WXGTK__
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glext.h"
#endif

#ifdef __WXOSX__
#include <OpenGL/gl3.h>  // from ..../Frameworks/OpenGL.framework/Headers/gl.h
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#endif

#ifdef WIN32
#define GL_GLEXT_LEGACY
#include "GL/gl.h"
#include "GL/glu.h"
#include "opengl/GL/glext.h"
#include <wx/glcanvas.h>
#endif

struct uniform_info {
  const char *name;
  GLuint size; /**< number of value[] elements: 1, 2, 3 or 4 */
  GLenum type; /**< GL_FLOAT, GL_FLOAT_VEC4, GL_INT, GL_FLOAT_MAT4, etc */
  GLfloat value[16];
  GLint location; /**< filled in by InitUniforms() */
};

#define END_OF_UNIFORMS \
  { NULL, 0, GL_NONE, {0, 0, 0, 0}, -1 }

struct attrib_info {
  const char *name;
  GLuint size; /**< number of value[] elements: 1, 2, 3 or 4 */
  GLenum type; /**< GL_FLOAT, GL_FLOAT_VEC4, GL_INT, etc */
  GLint location;
};

extern GLboolean ShadersSupported(void);

extern bool CompileShaderText(GLuint *shader, GLenum shaderType, const char *text);

extern GLuint LinkShaders(GLuint vertShader, GLuint fragShader);

extern GLuint LinkShaders3(GLuint vertShader, GLuint geomShader, GLuint fragShader);

extern GLuint LinkShaders3WithGeometryInfo(GLuint vertShader, GLuint geomShader, GLuint fragShader,
                                           GLint verticesOut, GLenum inputType, GLenum outputType);

extern GLboolean ValidateShaderProgram(GLuint program);

extern GLdouble GetShaderCompileTime(void);

extern GLdouble GetShaderLinkTime(void);

extern void SetUniformValues(GLuint program, struct uniform_info uniforms[]);

/*
 * These pointers are only valid after calling ShadersSupported.
 * Note that this includes this same header file recursively in a different mode!
 */
#define SHADER_FUNCTION_LIST(proc, name) extern proc name;
#include "shaderutil.h"
#undef SHADER_FUNCTION_LIST

#ifdef __cplusplus
}
#endif

#endif /* SHADER_UTIL_H */

#ifdef SHADER_FUNCTION_LIST
SHADER_FUNCTION_LIST(PFNGLCREATESHADERPROC, CreateShader)
SHADER_FUNCTION_LIST(PFNGLDELETESHADERPROC, DeleteShader)
SHADER_FUNCTION_LIST(PFNGLSHADERSOURCEPROC, ShaderSource)
SHADER_FUNCTION_LIST(PFNGLGETSHADERIVPROC, GetShaderiv)
SHADER_FUNCTION_LIST(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)
SHADER_FUNCTION_LIST(PFNGLCREATEPROGRAMPROC, CreateProgram)
SHADER_FUNCTION_LIST(PFNGLDELETEPROGRAMPROC, DeleteProgram)
SHADER_FUNCTION_LIST(PFNGLATTACHSHADERPROC, AttachShader)
SHADER_FUNCTION_LIST(PFNGLLINKPROGRAMPROC, LinkProgram)
SHADER_FUNCTION_LIST(PFNGLUSEPROGRAMPROC, UseProgram)
SHADER_FUNCTION_LIST(PFNGLGETPROGRAMIVPROC, GetProgramiv)
SHADER_FUNCTION_LIST(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog)
SHADER_FUNCTION_LIST(PFNGLVALIDATEPROGRAMPROC, ValidateProgram)
SHADER_FUNCTION_LIST(PFNGLUNIFORM1IPROC, Uniform1i)
SHADER_FUNCTION_LIST(PFNGLUNIFORM1FVPROC, Uniform1fv)
SHADER_FUNCTION_LIST(PFNGLUNIFORM2FVPROC, Uniform2fv)
SHADER_FUNCTION_LIST(PFNGLUNIFORM3FVPROC, Uniform3fv)
SHADER_FUNCTION_LIST(PFNGLUNIFORM4FVPROC, Uniform4fv)
SHADER_FUNCTION_LIST(PFNGLUNIFORMMATRIX4FVPROC, UniformMatrix4fv)
SHADER_FUNCTION_LIST(PFNGLGETACTIVEATTRIBPROC, GetActiveAttrib)
SHADER_FUNCTION_LIST(PFNGLGETATTRIBLOCATIONPROC, GetAttribLocation)
SHADER_FUNCTION_LIST(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation)
SHADER_FUNCTION_LIST(PFNGLGETACTIVEUNIFORMPROC, GetActiveUniform)
SHADER_FUNCTION_LIST(PFNGLCOMPILESHADERPROC, CompileShader)
#endif
