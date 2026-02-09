#pragma once

#include "app/Config.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <array>

namespace te {

class Input {
public:
  Input() = default;

  void Attach(GLFWwindow* window);
  void BeginFrame();
  void Update(GLFWwindow* window);

  bool IsKeyDown(int key) const;
  bool WasKeyPressed(int key) const;

  bool IsMouseDown(int button) const;
  bool WasMousePressed(int button) const;
  bool WasMouseReleased(int button) const;

  Vec2 GetMousePos() const { return m_mousePos; }
  Vec2 GetMouseDelta() const { return {m_mousePos.x - m_prevMousePos.x, m_mousePos.y - m_prevMousePos.y}; }
  Vec2 GetScrollDelta() const { return m_scrollDelta; }

private:
  static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

  std::array<unsigned char, GLFW_KEY_LAST + 1> m_keys{};
  std::array<unsigned char, GLFW_KEY_LAST + 1> m_prevKeys{};
  std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseButtons{};
  std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> m_prevMouseButtons{};

  Vec2 m_mousePos{};
  Vec2 m_prevMousePos{};
  Vec2 m_scrollDelta{};

  GLFWwindow* m_window = nullptr;

  static Input* s_instance;
};

} // namespace te
