#include "render/Framebuffer.h"

#include "render/GL.h"
#include "util/Log.h"

namespace te {

Framebuffer::~Framebuffer() {
  Destroy();
}

void Framebuffer::Resize(int width, int height) {
  if (width < 1 || height < 1) {
    Destroy();
    m_width = 0;
    m_height = 0;
    return;
  }

  if (m_width == width && m_height == height && m_fbo != 0) {
    return;
  }

  Destroy();
  m_width = width;
  m_height = height;

  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  glGenTextures(1, &m_colorTexture);
  glBindTexture(GL_TEXTURE_2D, m_colorTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

  glGenRenderbuffers(1, &m_depthRbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    Log::Error("Scene framebuffer incomplete.");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Framebuffer::Unbind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy() {
  if (m_depthRbo) {
    glDeleteRenderbuffers(1, &m_depthRbo);
    m_depthRbo = 0;
  }
  if (m_colorTexture) {
    glDeleteTextures(1, &m_colorTexture);
    m_colorTexture = 0;
  }
  if (m_fbo) {
    glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
  }
}

} // namespace te
