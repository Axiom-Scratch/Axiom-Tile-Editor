#include "render/Renderer2D.h"

#include "render/GL.h"
#include "util/Log.h"

namespace te {

bool Renderer2D::Init() {
  const char* vertexSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUv;

uniform mat4 u_ViewProj;

out vec4 vColor;
out vec2 vUv;

void main() {
  vColor = aColor;
  vUv = aUv;
  gl_Position = u_ViewProj * vec4(aPos, 0.0, 1.0);
}
)";

  const char* fragmentSrc = R"(
#version 330 core
in vec4 vColor;
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D u_Texture;
uniform int u_UseTexture;

void main() {
  vec4 color = vColor;
  if (u_UseTexture == 1) {
    color *= texture(u_Texture, vUv);
  }
  FragColor = color;
}
)";

  if (!m_shader.LoadFromSource(vertexSrc, fragmentSrc)) {
    return false;
  }
  m_shader.Bind();
  m_shader.SetInt("u_Texture", 0);

  m_quadMesh.Create();
  m_lineMesh.Create();

  m_quadVertices.reserve(MaxQuadVertices);
  m_lineVertices.reserve(MaxLineVertices);
  m_quadIndices.reserve(MaxQuadIndices);

  m_quadIndices.clear();
  for (size_t i = 0; i < MaxQuads; ++i) {
    unsigned int offset = static_cast<unsigned int>(i * 4);
    m_quadIndices.push_back(offset + 0);
    m_quadIndices.push_back(offset + 1);
    m_quadIndices.push_back(offset + 2);
    m_quadIndices.push_back(offset + 2);
    m_quadIndices.push_back(offset + 3);
    m_quadIndices.push_back(offset + 0);
  }

  m_quadMesh.Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_quadMesh.GetVbo());
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(MaxQuadVertices * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadMesh.GetEbo());
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_quadIndices.size() * sizeof(unsigned int)),
               m_quadIndices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 2));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 6));
  m_quadMesh.Unbind();

  m_lineMesh.Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_lineMesh.GetVbo());
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(MaxLineVertices * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 2));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(sizeof(float) * 6));
  m_lineMesh.Unbind();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return true;
}

void Renderer2D::Shutdown() {
  m_quadMesh.Destroy();
  m_lineMesh.Destroy();
}

void Renderer2D::BeginFrame(const Mat4& viewProj) {
  m_viewProj = viewProj;
  m_quadVertices.clear();
  m_lineVertices.clear();
  m_activeTexture = nullptr;
}

void Renderer2D::DrawQuad(const Vec2& position, const Vec2& size, const Vec4& color) {
  DrawQuad(position, size, color, {0.0f, 0.0f}, {1.0f, 1.0f}, nullptr);
}

void Renderer2D::DrawQuad(const Vec2& position, const Vec2& size, const Vec4& color,
                          const Vec2& uv0, const Vec2& uv1, const Texture* texture) {
  if (texture != m_activeTexture && !m_quadVertices.empty()) {
    FlushQuads();
    m_quadVertices.clear();
  }
  m_activeTexture = texture;

  if (m_quadVertices.size() + 4 > MaxQuadVertices) {
    FlushQuads();
    m_quadVertices.clear();
  }

  const float x = position.x;
  const float y = position.y;
  const float w = size.x;
  const float h = size.y;

  m_quadVertices.push_back({x, y, color.r, color.g, color.b, color.a, uv0.x, uv0.y});
  m_quadVertices.push_back({x + w, y, color.r, color.g, color.b, color.a, uv1.x, uv0.y});
  m_quadVertices.push_back({x + w, y + h, color.r, color.g, color.b, color.a, uv1.x, uv1.y});
  m_quadVertices.push_back({x, y + h, color.r, color.g, color.b, color.a, uv0.x, uv1.y});
}

void Renderer2D::DrawLine(const Vec2& a, const Vec2& b, const Vec4& color) {
  if (m_lineVertices.size() + 2 > MaxLineVertices) {
    FlushLines();
    m_lineVertices.clear();
  }

  m_lineVertices.push_back({a.x, a.y, color.r, color.g, color.b, color.a, 0.0f, 0.0f});
  m_lineVertices.push_back({b.x, b.y, color.r, color.g, color.b, color.a, 0.0f, 0.0f});
}

void Renderer2D::EndFrame() {
  FlushQuads();
  FlushLines();
}

void Renderer2D::FlushQuads() {
  if (m_quadVertices.empty()) {
    return;
  }

  m_shader.Bind();
  m_shader.SetMat4("u_ViewProj", m_viewProj);
  m_shader.SetInt("u_UseTexture", (m_activeTexture && m_activeTexture->IsValid()) ? 1 : 0);
  if (m_activeTexture && m_activeTexture->IsValid()) {
    m_activeTexture->Bind(0);
  }

  m_quadMesh.Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_quadMesh.GetVbo());
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_quadVertices.size() * sizeof(Vertex)),
                  m_quadVertices.data());

  const size_t quadCount = m_quadVertices.size() / 4;
  const size_t indexCount = quadCount * 6;
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
  m_quadMesh.Unbind();
}

void Renderer2D::FlushLines() {
  if (m_lineVertices.empty()) {
    return;
  }

  m_shader.Bind();
  m_shader.SetMat4("u_ViewProj", m_viewProj);
  m_shader.SetInt("u_UseTexture", 0);

  m_lineMesh.Bind();
  glBindBuffer(GL_ARRAY_BUFFER, m_lineMesh.GetVbo());
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_lineVertices.size() * sizeof(Vertex)),
                  m_lineVertices.data());

  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineVertices.size()));
  m_lineMesh.Unbind();
}

} // namespace te
