#include "render/Mesh.h"

#include "render/GL.h"

namespace te {

Mesh::~Mesh() {
  Destroy();
}

void Mesh::Create() {
  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);
}

void Mesh::Destroy() {
  if (m_ebo != 0) {
    glDeleteBuffers(1, &m_ebo);
    m_ebo = 0;
  }
  if (m_vbo != 0) {
    glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
  }
  if (m_vao != 0) {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
  }
}

void Mesh::Bind() const {
  glBindVertexArray(m_vao);
}

void Mesh::Unbind() const {
  glBindVertexArray(0);
}

} // namespace te
