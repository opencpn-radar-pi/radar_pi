/**
 * Utilities for OpenGL shading language
 * Modified for opencpn Sean D'Epagnier 2015
 *
 * Brian Paul
 * 9 April 2008
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shaderutil.h"

PFNGLCREATESHADERPROC CreateShader = NULL;
PFNGLDELETESHADERPROC DeleteShader = NULL;
PFNGLSHADERSOURCEPROC ShaderSource = NULL;
PFNGLGETSHADERIVPROC GetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC CreateProgram = NULL;
PFNGLDELETEPROGRAMPROC DeleteProgram = NULL;
//PFNGLATTACHSHADERPROC AttachShader = NULL;
PFNGLLINKPROGRAMPROC LinkProgram = NULL;
PFNGLUSEPROGRAMPROC UseProgram = NULL;
PFNGLGETPROGRAMIVPROC GetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog = NULL;
PFNGLVALIDATEPROGRAMPROC ValidateProgram = NULL;
PFNGLUNIFORM1IPROC Uniform1i = NULL;
PFNGLUNIFORM1FVPROC Uniform1fv = NULL;
PFNGLUNIFORM2FVPROC Uniform2fv = NULL;
PFNGLUNIFORM3FVPROC Uniform3fv = NULL;
PFNGLUNIFORM4FVPROC Uniform4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv = NULL;
PFNGLGETACTIVEATTRIBPROC GetActiveAttrib = NULL;
PFNGLGETATTRIBLOCATIONPROC GetAttribLocation = NULL;
PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation = NULL;
PFNGLGETACTIVEUNIFORMPROC GetActiveUniform = NULL;
PFNGLCOMPILESHADERPROC CompileShader = NULL;

#if 0
static void GLAPIENTRY
fake_ValidateProgram(GLuint prog)
{
   (void) prog;
}
#endif

#if defined(__WXMSW__)
#define systemGetProcAddress(ADDR) wglGetProcAddress(ADDR)
#elif defined(__WXOSX__)
#include <dlfcn.h>
#define systemGetProcAddress(ADDR) dlsym( RTLD_DEFAULT, ADDR)
#else
#include <GL/glx.h>
#define systemGetProcAddress(ADDR) glXGetProcAddress((const GLubyte*)ADDR)
#endif

GLboolean
ShadersSupported(void)
{
    CreateShader = systemGetProcAddress("glCreateShader");
    DeleteShader = systemGetProcAddress("glDeleteShader");
    ShaderSource = systemGetProcAddress("glShaderSource");
    GetShaderiv = systemGetProcAddress("glGetShaderiv");
    GetShaderInfoLog = systemGetProcAddress("glGetShaderInfoLog");
    CreateProgram = systemGetProcAddress("glCreateProgram");
    DeleteProgram = systemGetProcAddress("glDeleteProgram");
    //AttachShader = systemGetProcAddress("glAttachShader");
    LinkProgram = systemGetProcAddress("glLinkProgram");
    UseProgram = systemGetProcAddress("glUseProgram");
    GetProgramiv = systemGetProcAddress("glGetProgramiv");
    GetProgramInfoLog = systemGetProcAddress("glGetProgramInfoLog");
    ValidateProgram =  systemGetProcAddress("glValidateProgram");
    Uniform1i = systemGetProcAddress("glUniform1i");
    Uniform1fv = systemGetProcAddress("glUniform1fv");
    Uniform2fv = systemGetProcAddress("glUniform2fv");
    Uniform3fv = systemGetProcAddress("glUniform3fv");
    Uniform4fv = systemGetProcAddress("glUniform4fv");
    UniformMatrix4fv = systemGetProcAddress("glUniformMatrix4fv");
    GetActiveAttrib = systemGetProcAddress("glGetActiveAttrib");
    GetAttribLocation = systemGetProcAddress("glGetAttribLocation");
    GetUniformLocation = systemGetProcAddress("glGetUniformLocation");
    GetActiveUniform = systemGetProcAddress("glGetActiveUniform");
    CompileShader = systemGetProcAddress("glCompileShader");

    return UseProgram != 0;
}

GLuint
CompileShaderText(GLenum shaderType, const char *text)
{
   GLuint shader;
   GLint stat;

   shader = CreateShader(shaderType);
   ShaderSource(shader, 1, (const GLchar **) &text, NULL);

   CompileShader(shader);

   GetShaderiv(shader, GL_COMPILE_STATUS, &stat);
   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      GetShaderInfoLog(shader, 1000, &len, log);
      fprintf(stderr, "Error: problem compiling shader: %s\n", log);
      exit(1);
   }
   else {
      /*printf("Shader compiled OK\n");*/
   }
   return shader;
}


/**
 * Read a shader from a file.
 */
GLuint
CompileShaderFile(GLenum shaderType, const char *filename)
{
   const int max = 100*1000;
   int n;
   char *buffer = (char*) malloc(max);
   GLuint shader;
   FILE *f;

   f = fopen(filename, "r");
   if (!f) {
      fprintf(stderr, "Unable to open shader file %s\n", filename);
      free(buffer);
      return 0;
   }

   n = fread(buffer, 1, max, f);
   /*printf("read %d bytes from shader file %s\n", n, filename);*/
   if (n > 0) {
      buffer[n] = 0;
      shader = CompileShaderText(shaderType, buffer);
   }
   else {
      fclose(f);
      free(buffer);
      return 0;
   }

   fclose(f);
   free(buffer);

   return shader;
}


GLuint
LinkShaders(GLuint vertShader, GLuint fragShader)
{
   return LinkShaders3(vertShader, 0, fragShader);
}

#if 0
GLuint
LinkShaders3(GLuint vertShader, GLuint geomShader, GLuint fragShader)
{
   GLuint program = CreateProgram();

   assert(vertShader || fragShader);

   if (vertShader)
      AttachShader(program, vertShader);
   if (geomShader)
      AttachShader(program, geomShader);
   if (fragShader)
      AttachShader(program, fragShader);

   LinkProgram(program);

   /* check link */
   {
      GLint stat;
      GetProgramiv(program, GL_LINK_STATUS, &stat);
      if (!stat) {
         GLchar log[1000];
         GLsizei len;
         GetProgramInfoLog(program, 1000, &len, log);
         fprintf(stderr, "Shader link error:\n%s\n", log);
         return 0;
      }
   }

   return program;
}


GLuint
LinkShaders3WithGeometryInfo(GLuint vertShader, GLuint geomShader, GLuint fragShader,
                             GLint verticesOut, GLenum inputType, GLenum outputType)
{
  GLuint program = CreateProgram();

  assert(vertShader || fragShader);

  if (vertShader)
    AttachShader(program, vertShader);
  if (geomShader) {
    AttachShader(program, geomShader);
    ProgramParameteriARB(program, GL_GEOMETRY_VERTICES_OUT_ARB, verticesOut);
    ProgramParameteriARB(program, GL_GEOMETRY_INPUT_TYPE_ARB, inputType);
    ProgramParameteriARB(program, GL_GEOMETRY_OUTPUT_TYPE_ARB, outputType);
  }
  if (fragShader)
    AttachShader(program, fragShader);

  LinkProgram(program);


  /* check link */
  {
    GLint stat;
    GetProgramiv(program, GL_LINK_STATUS, &stat);
    if (!stat) {
      GLchar log[1000];
      GLsizei len;
      GetProgramInfoLog(program, 1000, &len, log);
      fprintf(stderr, "Shader link error:\n%s\n", log);
      return 0;
    }
  }

  return program;
}
#endif


GLboolean
ValidateShaderProgram(GLuint program)
{
   GLint stat;
   ValidateProgram(program);
   GetProgramiv(program, GL_VALIDATE_STATUS, &stat);

   if (!stat) {
      GLchar log[1000];
      GLsizei len;
      GetProgramInfoLog(program, 1000, &len, log);
      fprintf(stderr, "Program validation error:\n%s\n", log);
      return 0;
   }

   return (GLboolean) stat;
}

void
SetUniformValues(GLuint program, struct uniform_info uniforms[])
{
   GLuint i;

   for (i = 0; uniforms[i].name; i++) {
      uniforms[i].location
         = GetUniformLocation(program, uniforms[i].name);

      switch (uniforms[i].type) {
      case GL_INT:
      case GL_SAMPLER_1D:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_2D_RECT:
      case GL_SAMPLER_1D_SHADOW:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_1D_ARRAY:
      case GL_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_1D_ARRAY_SHADOW:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
         assert(uniforms[i].value[0] >= 0.0F);
         Uniform1i(uniforms[i].location,
                     (GLint) uniforms[i].value[0]);
         break;
      case GL_FLOAT:
         Uniform1fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC2:
         Uniform2fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC3:
         Uniform3fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_VEC4:
         Uniform4fv(uniforms[i].location, 1, uniforms[i].value);
         break;
      case GL_FLOAT_MAT4:
         UniformMatrix4fv(uniforms[i].location, 1, GL_FALSE,
                          uniforms[i].value);
         break;
      default:
         if (strncmp(uniforms[i].name, "gl_", 3) == 0) {
            /* built-in uniform: ignore */
         }
         else {
            fprintf(stderr,
                    "Unexpected uniform data type in SetUniformValues\n");
            abort();
         }
      }
   }
}


/** Get list of uniforms used in the program */
GLuint
GetUniforms(GLuint program, struct uniform_info uniforms[])
{
   GLint n, max, i;

   GetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);
   GetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      GetActiveUniform(program, i, 100, &len, &size, &type, name);

      uniforms[i].name = strdup(name);
      uniforms[i].size = size;
      uniforms[i].type = type;
      uniforms[i].location = GetUniformLocation(program, name);
   }

   uniforms[i].name = NULL; /* end of list */

   return n;
}


void
PrintUniforms(const struct uniform_info uniforms[])
{
   GLint i;

   printf("Uniforms:\n");

   for (i = 0; uniforms[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d value=%g, %g, %g, %g\n",
             i,
             uniforms[i].name,
             uniforms[i].size,
             uniforms[i].type,
             uniforms[i].location,
             uniforms[i].value[0],
             uniforms[i].value[1],
             uniforms[i].value[2],
             uniforms[i].value[3]);
   }
}


/** Get list of attribs used in the program */
GLuint
GetAttribs(GLuint program, struct attrib_info attribs[])
{
   GLint n, max, i;

   GetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
   GetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max);

   for (i = 0; i < n; i++) {
      GLint size, len;
      GLenum type;
      char name[100];

      GetActiveAttrib(program, i, 100, &len, &size, &type, name);

      attribs[i].name = strdup(name);
      attribs[i].size = size;
      attribs[i].type = type;
      attribs[i].location = GetAttribLocation(program, name);
   }

   attribs[i].name = NULL; /* end of list */

   return n;
}


void
PrintAttribs(const struct attrib_info attribs[])
{
   GLint i;

   printf("Attribs:\n");

   for (i = 0; attribs[i].name; i++) {
      printf("  %d: %s size=%d type=0x%x loc=%d\n",
             i,
             attribs[i].name,
             attribs[i].size,
             attribs[i].type,
             attribs[i].location);
   }
}

#ifdef __cplusplus
}
#endif
