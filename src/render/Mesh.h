#pragma once

namespace te {

class Mesh {
public:
  Mesh() = default;
  ~Mesh();

  void Create();
  void Destroy();

  void Bind() const;
  void Unbind() const;

  unsigned int GetVao() const { return m_vao; }
  unsigned int GetVbo() const { return m_vbo; }
  unsigned int GetEbo() const { return m_ebo; }

private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
};

} // namespace te
