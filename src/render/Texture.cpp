#include "render/Texture.h"

#include "render/GL.h"
#include "util/Log.h"

#include <stb_image.h>

namespace te {

Texture::~Texture() {
  Destroy();
}

bool Texture::LoadFromFile(const std::string& path, bool flipVertical) {
  Destroy();

  stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
  unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 4);
  if (!data) {
    Log::Error("Failed to load texture: " + path);
    unsigned char fallback[4] = {255, 0, 255, 255};
    m_width = 1;
    m_height = 1;
    m_channels = 4;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallback);
    m_isFallback = true;
    return false;
  }

  glGenTextures(1, &m_id);
  glBindTexture(GL_TEXTURE_2D, m_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);
  m_isFallback = false;
  return true;
}

void Texture::Destroy() {
  if (m_id != 0) {
    glDeleteTextures(1, &m_id);
    m_id = 0;
  }
  m_width = 0;
  m_height = 0;
  m_channels = 0;
  m_isFallback = false;
}

void Texture::Bind(int slot) const {
  glActiveTexture(static_cast<unsigned int>(GL_TEXTURE0 + slot));
  glBindTexture(GL_TEXTURE_2D, m_id);
}

} // namespace te
