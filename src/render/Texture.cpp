#include "render/Texture.h"

#include "render/GL.h"
#include "util/Log.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stb_image.h>

namespace te {

Texture::~Texture() {
  Destroy();
}

bool Texture::LoadFromFile(const std::string& path, bool flipVertical) {
  Destroy();

  stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
  bool isLfsPointer = false;
  std::string headerString;
  {
    std::ifstream file(path, std::ios::binary);
    if (file) {
      char header[64] = {};
      file.read(header, static_cast<std::streamsize>(sizeof(header)));
      const std::streamsize bytesRead = file.gcount();
      headerString.assign(header, header + bytesRead);
      if (headerString.rfind("version https://git-lfs.github.com/spec", 0) == 0) {
        isLfsPointer = true;
      }
    }
  }

  unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 4);
  if (!data) {
    static bool s_loggedFailure = false;
    if (!s_loggedFailure) {
      s_loggedFailure = true;
      std::error_code ec;
      const std::filesystem::path cwd = std::filesystem::current_path(ec);
      const std::filesystem::path absPath = std::filesystem::absolute(path, ec);
      const bool exists = std::filesystem::exists(absPath, ec);
      const auto fileSize = std::filesystem::file_size(absPath, ec);
      unsigned char headerBytes[8] = {};
      std::ifstream file(absPath, std::ios::binary);
      if (file) {
        file.read(reinterpret_cast<char*>(headerBytes), static_cast<std::streamsize>(sizeof(headerBytes)));
      }
      std::ostringstream hex;
      hex << std::hex << std::setfill('0');
      for (unsigned char b : headerBytes) {
        hex << std::setw(2) << static_cast<int>(b);
      }
      Log::Error("Texture load failed.");
      Log::Error("CWD: " + cwd.string());
      Log::Error("Absolute path: " + absPath.string());
      Log::Error(std::string("Exists: ") + (exists ? "true" : "false"));
      Log::Error(std::string("File size: ") + (ec ? "unknown" : std::to_string(fileSize)));
      Log::Error("Header (first 8 bytes): " + hex.str());
      const char* reason = stbi_failure_reason();
      Log::Error(std::string("stb_image failure: ") + (reason ? reason : "unknown"));
      if (isLfsPointer) {
        Log::Error("atlas.png is a Git LFS pointer file; install git-lfs and run git lfs pull, or replace atlas.png with a real PNG.");
      }
      Log::Error("Failed to load texture: " + path);
    }
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

  if (m_width <= 0 || m_height <= 0) {
    stbi_image_free(data);
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
