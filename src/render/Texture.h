#pragma once

#include <string>

namespace te {

class Texture {
public:
  Texture() = default;
  ~Texture();

  bool LoadFromFile(const std::string& path, bool flipVertical = true);
  void Destroy();

  void Bind(int slot = 0) const;

  bool IsValid() const { return m_id != 0; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }

private:
  unsigned int m_id = 0;
  int m_width = 0;
  int m_height = 0;
  int m_channels = 0;
};

} // namespace te
