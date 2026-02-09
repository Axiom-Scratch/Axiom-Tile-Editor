#pragma once

#include <glad/glad.h>

#include "util/Log.h"

#include <string>

namespace te::gl {

inline void EnableDebugOutput() {
  if (!glDebugMessageCallback) {
    Log::Warn("OpenGL debug output not available.");
    return;
  }

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(
      [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
         const void* userParam) {
        (void)source;
        (void)type;
        (void)id;
        (void)length;
        (void)userParam;

        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
          return;
        }
        Log::Warn(std::string("GL Debug: ") + message);
      },
      nullptr);
}

} // namespace te::gl
