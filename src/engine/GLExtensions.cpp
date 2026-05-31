#include "GLExtensions.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32

PFNGLCREATESHADERPROC       glCreateShader       = nullptr;
PFNGLSHADERSOURCEPROC       glShaderSource       = nullptr;
PFNGLCOMPILESHADERPROC      glCompileShader      = nullptr;
PFNGLGETSHADERIVPROC        glGetShaderiv        = nullptr;
PFNGLGETSHADERINFOLOGPROC   glGetShaderInfoLog   = nullptr;
PFNGLDELETESHADERPROC       glDeleteShader       = nullptr;
PFNGLCREATEPROGRAMPROC      glCreateProgram      = nullptr;
PFNGLATTACHSHADERPROC       glAttachShader       = nullptr;
PFNGLLINKPROGRAMPROC        glLinkProgram        = nullptr;
PFNGLGETPROGRAMIVPROC       glGetProgramiv       = nullptr;
PFNGLGETPROGRAMINFOLOGPROC  glGetProgramInfoLog  = nullptr;
PFNGLDELETEPROGRAMPROC      glDeleteProgram      = nullptr;
PFNGLUSEPROGRAMPROC         glUseProgram         = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC          glUniform1i          = nullptr;
PFNGLUNIFORM1FPROC          glUniform1f          = nullptr;

#define LOAD_GL_FUNC(type, name) \
    name = (type)wglGetProcAddress(#name); \
    if (!name) { printf("[GL] Failed to load: " #name "\n"); return false; }

bool initGLExtensions() {
    LOAD_GL_FUNC(PFNGLCREATESHADERPROC,       glCreateShader)
    LOAD_GL_FUNC(PFNGLSHADERSOURCEPROC,       glShaderSource)
    LOAD_GL_FUNC(PFNGLCOMPILESHADERPROC,      glCompileShader)
    LOAD_GL_FUNC(PFNGLGETSHADERIVPROC,        glGetShaderiv)
    LOAD_GL_FUNC(PFNGLGETSHADERINFOLOGPROC,   glGetShaderInfoLog)
    LOAD_GL_FUNC(PFNGLDELETESHADERPROC,       glDeleteShader)
    LOAD_GL_FUNC(PFNGLCREATEPROGRAMPROC,      glCreateProgram)
    LOAD_GL_FUNC(PFNGLATTACHSHADERPROC,       glAttachShader)
    LOAD_GL_FUNC(PFNGLLINKPROGRAMPROC,        glLinkProgram)
    LOAD_GL_FUNC(PFNGLGETPROGRAMIVPROC,       glGetProgramiv)
    LOAD_GL_FUNC(PFNGLGETPROGRAMINFOLOGPROC,  glGetProgramInfoLog)
    LOAD_GL_FUNC(PFNGLDELETEPROGRAMPROC,      glDeleteProgram)
    LOAD_GL_FUNC(PFNGLUSEPROGRAMPROC,         glUseProgram)
    LOAD_GL_FUNC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)
    LOAD_GL_FUNC(PFNGLUNIFORM1IPROC,          glUniform1i)
    LOAD_GL_FUNC(PFNGLUNIFORM1FPROC,          glUniform1f)
    return true;
}

#endif