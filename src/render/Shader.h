#pragma once

#include "app/Config.h"

#include <string>

namespace te {

class Shader {
public:
  Shader() = default;
  ~Shader();

  bool LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
  void Bind() const;
  void Unbind() const;

  void SetMat4(const std::string& name, const Mat4& value) const;
  void SetVec4(const std::string& name, const Vec4& value) const;
  void SetInt(const std::string& name, int value) const;

private:
  unsigned int m_program = 0;

  int GetUniformLocation(const std::string& name) const;
};

} // namespace te
