#pragma once

#include <glad/glad.h>

namespace te {

class Framebuffer {
public:
  Framebuffer() = default;
  ~Framebuffer();

  void Resize(int width, int height);
  void Bind() const;
  void Unbind() const;

  GLuint GetColorTexture() const { return m_colorTexture; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }

private:
  void Destroy();

  GLuint m_fbo = 0;
  GLuint m_colorTexture = 0;
  GLuint m_depthRbo = 0;
  int m_width = 0;
  int m_height = 0;
};

} // namespace te
