#include <glad/glad.h>

#include <stddef.h>

PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;

PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;

PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray = NULL;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;

PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;

PFNGLDEBUGMESSAGECALLBACKPROC glad_glDebugMessageCallback = NULL;
PFNGLDEBUGMESSAGECONTROLPROC glad_glDebugMessageControl = NULL;

int gladLoadGLLoader(GLADloadproc load) {
  if (!load) {
    return 0;
  }

  glad_glEnable = (PFNGLENABLEPROC)load("glEnable");
  glad_glDisable = (PFNGLDISABLEPROC)load("glDisable");
  glad_glBlendFunc = (PFNGLBLENDFUNCPROC)load("glBlendFunc");
  glad_glClearColor = (PFNGLCLEARCOLORPROC)load("glClearColor");
  glad_glClear = (PFNGLCLEARPROC)load("glClear");
  glad_glViewport = (PFNGLVIEWPORTPROC)load("glViewport");
  glad_glGetIntegerv = (PFNGLGETINTEGERVPROC)load("glGetIntegerv");
  glad_glGetString = (PFNGLGETSTRINGPROC)load("glGetString");

  glad_glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
  glad_glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
  glad_glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
  glad_glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
  glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
  glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
  glad_glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
  glad_glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
  glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
  glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");
  glad_glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");
  glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
  glad_glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
  glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load("glGetUniformLocation");
  glad_glUniform1i = (PFNGLUNIFORM1IPROC)load("glUniform1i");
  glad_glUniform4f = (PFNGLUNIFORM4FPROC)load("glUniform4f");
  glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load("glUniformMatrix4fv");

  glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
  glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
  glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load("glDeleteVertexArrays");
  glad_glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
  glad_glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
  glad_glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
  glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC)load("glBufferSubData");
  glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load("glDeleteBuffers");
  glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
  glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
  glad_glDrawElements = (PFNGLDRAWELEMENTSPROC)load("glDrawElements");
  glad_glDrawArrays = (PFNGLDRAWARRAYSPROC)load("glDrawArrays");

  glad_glGenTextures = (PFNGLGENTEXTURESPROC)load("glGenTextures");
  glad_glBindTexture = (PFNGLBINDTEXTUREPROC)load("glBindTexture");
  glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC)load("glTexParameteri");
  glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC)load("glTexImage2D");
  glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)load("glGenerateMipmap");
  glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC)load("glDeleteTextures");
  glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC)load("glActiveTexture");

  glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)load("glDebugMessageCallback");
  glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)load("glDebugMessageControl");

  if (!glad_glGetString || !glad_glClear || !glad_glCreateShader || !glad_glCreateProgram || !glad_glGenBuffers ||
      !glad_glGenVertexArrays || !glad_glDrawArrays || !glad_glDrawElements) {
    return 0;
  }

  return 1;
}
