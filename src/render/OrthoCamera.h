#pragma once

#include "app/Config.h"

namespace te {

class OrthoCamera {
public:
  void SetPosition(const Vec2& position) { m_position = position; }
  void SetZoom(float zoom) { m_zoom = zoom; }

  const Vec2& GetPosition() const { return m_position; }
  float GetZoom() const { return m_zoom; }

  Mat4 GetViewProjection(const Vec2i& viewport) const;
  Vec2 ScreenToWorld(const Vec2& screenPos, const Vec2i& viewport) const;

private:
  Vec2 m_position{0.0f, 0.0f};
  float m_zoom = 1.0f;
};

} // namespace te
