#pragma once

#include <cstdint>

namespace te {

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

struct Vec2i {
  int x = 0;
  int y = 0;
};

struct Vec4 {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct Mat4 {
  float m[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
};

struct AppConfig {
  static constexpr const char* WindowTitle = "Tile Editor";
  static constexpr int WindowWidth = 1280;
  static constexpr int WindowHeight = 720;

  static constexpr int MapWidth = 32;
  static constexpr int MapHeight = 32;
  static constexpr int TileSize = 32;
};

} // namespace te
