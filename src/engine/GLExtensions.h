#pragma once

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
typedef char GLchar;
#define GL_VERTEX_SHADER   0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

// Types des fonctions shader
typedef GLuint (APIENTRY* PFNGLCREATESHADERPROC)(GLenum);
typedef void   (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void   (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint);
typedef void   (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY* PFNGLDELETESHADERPROC)(GLuint);
typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void   (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void   (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint);
typedef void   (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void   (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint);
typedef GLint  (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void   (APIENTRY* PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void   (APIENTRY* PFNGLUNIFORM1FPROC)(GLint, GLfloat);

extern PFNGLCREATESHADERPROC       glCreateShader;
extern PFNGLSHADERSOURCEPROC       glShaderSource;
extern PFNGLCOMPILESHADERPROC      glCompileShader;
extern PFNGLGETSHADERIVPROC        glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC       glDeleteShader;
extern PFNGLCREATEPROGRAMPROC      glCreateProgram;
extern PFNGLATTACHSHADERPROC       glAttachShader;
extern PFNGLLINKPROGRAMPROC        glLinkProgram;
extern PFNGLGETPROGRAMIVPROC       glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC      glDeleteProgram;
extern PFNGLUSEPROGRAMPROC         glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC          glUniform1i;
extern PFNGLUNIFORM1FPROC          glUniform1f;

bool initGLExtensions();

#else
// macOS / Linux : les fonctions sont disponibles directement
inline bool initGLExtensions() { return true; }
#endif