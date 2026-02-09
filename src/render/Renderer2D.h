#pragma once

#include "app/Config.h"

#include "render/Mesh.h"
#include "render/Shader.h"

#include <vector>

namespace te {

class Renderer2D {
public:
  bool Init();
  void Shutdown();

  void BeginFrame(const Mat4& viewProj);
  void DrawQuad(const Vec2& position, const Vec2& size, const Vec4& color);
  void DrawLine(const Vec2& a, const Vec2& b, const Vec4& color);
  void EndFrame();

private:
  struct Vertex {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  void FlushQuads();
  void FlushLines();

  Shader m_shader;
  Mesh m_quadMesh;
  Mesh m_lineMesh;
  Mat4 m_viewProj{};

  std::vector<Vertex> m_quadVertices;
  std::vector<Vertex> m_lineVertices;
  std::vector<unsigned int> m_quadIndices;

  static constexpr size_t MaxQuads = 10000;
  static constexpr size_t MaxQuadVertices = MaxQuads * 4;
  static constexpr size_t MaxQuadIndices = MaxQuads * 6;
  static constexpr size_t MaxLineVertices = 20000;
};

} // namespace te
