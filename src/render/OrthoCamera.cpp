#include "render/OrthoCamera.h"

namespace te {

namespace {

Mat4 MakeOrtho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
  Mat4 result{};
  const float rl = right - left;
  const float tb = top - bottom;
  const float fn = farPlane - nearPlane;

  result.m[0] = 2.0f / rl;
  result.m[5] = 2.0f / tb;
  result.m[10] = -2.0f / fn;
  result.m[12] = -(right + left) / rl;
  result.m[13] = -(top + bottom) / tb;
  result.m[14] = -(farPlane + nearPlane) / fn;
  result.m[15] = 1.0f;
  return result;
}

} // namespace

Mat4 OrthoCamera::GetViewProjection(const Vec2i& viewport) const {
  const float halfW = (viewport.x > 0 ? static_cast<float>(viewport.x) : 1.0f) * 0.5f / m_zoom;
  const float halfH = (viewport.y > 0 ? static_cast<float>(viewport.y) : 1.0f) * 0.5f / m_zoom;

  const float left = m_position.x - halfW;
  const float right = m_position.x + halfW;
  const float bottom = m_position.y - halfH;
  const float top = m_position.y + halfH;

  return MakeOrtho(left, right, bottom, top, -1.0f, 1.0f);
}

Vec2 OrthoCamera::ScreenToWorld(const Vec2& screenPos, const Vec2i& viewport) const {
  Vec2 world{};
  if (viewport.x <= 0 || viewport.y <= 0) {
    return world;
  }

  const float halfW = static_cast<float>(viewport.x) * 0.5f / m_zoom;
  const float halfH = static_cast<float>(viewport.y) * 0.5f / m_zoom;

  const float left = m_position.x - halfW;
  const float right = m_position.x + halfW;
  const float bottom = m_position.y - halfH;
  const float top = m_position.y + halfH;

  const float nx = screenPos.x / static_cast<float>(viewport.x);
  const float ny = screenPos.y / static_cast<float>(viewport.y);

  world.x = left + nx * (right - left);
  world.y = top - ny * (top - bottom);
  return world;
}

} // namespace te
