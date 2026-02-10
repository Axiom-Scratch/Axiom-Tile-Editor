#pragma once

#include "app/Config.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <array>

namespace te {

class Actions;

class Input {
public:
  Input() = default;

  void SetActions(Actions* actions);
  void Attach(GLFWwindow* window);
  void BeginFrame();
  void Update(GLFWwindow* window);

  void OnMouseMove(double x, double y);
  void OnMouseButton(int button, int action, int mods);
  void OnScroll(double xoff, double yoff);

  bool IsKeyDown(int key) const;
  bool WasKeyPressed(int key) const;

  bool IsMouseDown(int button) const;
  bool WasMousePressed(int button) const;
  bool WasMouseReleased(int button) const;

  Vec2 GetMousePos() const { return m_mousePos; }
  Vec2 GetMouseDelta() const { return m_mouseDelta; }
  Vec2 GetScrollDelta() const { return m_scrollDelta; }

private:
  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

  std::array<unsigned char, GLFW_KEY_LAST + 1> m_keys{};
  std::array<unsigned char, GLFW_KEY_LAST + 1> m_prevKeys{};
  std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseButtons{};
  std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> m_mousePressed{};
  std::array<unsigned char, GLFW_MOUSE_BUTTON_LAST + 1> m_mouseReleased{};

  Vec2 m_mousePos{};
  Vec2 m_mouseDelta{};
  Vec2 m_scrollDelta{};

  GLFWwindow* m_window = nullptr;
  Actions* m_actions = nullptr;

  static Input* s_instance;
};

} // namespace te
