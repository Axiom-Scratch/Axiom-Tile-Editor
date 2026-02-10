#include "platform/Input.h"

#include "platform/Actions.h"

#include <cassert>

namespace te {

Input* Input::s_instance = nullptr;

void Input::SetActions(Actions* actions) {
  m_actions = actions;
}

void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)window;
  (void)scancode;
  (void)mods;
  Input* input = window ? static_cast<Input*>(glfwGetWindowUserPointer(window)) : nullptr;
  if (!input) {
    input = s_instance;
  }
  if (!input) {
    return;
  }
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
#if defined(TE_DEBUG)
    assert(false && "Invalid GLFW key value.");
#endif
    return;
  }

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    input->m_keys[static_cast<size_t>(key)] = 1U;
  } else if (action == GLFW_RELEASE) {
    input->m_keys[static_cast<size_t>(key)] = 0U;
  }

  if (input->m_actions) {
    input->m_actions->OnKey(key, action);
  }
}

void Input::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  Input* input = window ? static_cast<Input*>(glfwGetWindowUserPointer(window)) : nullptr;
  if (!input) {
    input = s_instance;
  }
  if (!input) {
    return;
  }
  input->OnMouseMove(xpos, ypos);
}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  (void)window;
  Input* input = window ? static_cast<Input*>(glfwGetWindowUserPointer(window)) : nullptr;
  if (!input) {
    input = s_instance;
  }
  if (!input) {
    return;
  }
  input->OnMouseButton(button, action, mods);
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)window;
  Input* input = window ? static_cast<Input*>(glfwGetWindowUserPointer(window)) : nullptr;
  if (!input) {
    input = s_instance;
  }
  if (!input) {
    return;
  }
  input->OnScroll(xoffset, yoffset);
}

void Input::Attach(GLFWwindow* window) {
  m_window = window;
  s_instance = this;
  if (m_window) {
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
  }
}

void Input::BeginFrame() {
  m_prevKeys = m_keys;
  m_mouseDelta = {};
  m_scrollDelta = {};
  m_mousePressed.fill(0U);
  m_mouseReleased.fill(0U);
}

void Input::Update(GLFWwindow* window) {
  (void)window;
}

void Input::OnMouseMove(double x, double y) {
  const Vec2 newPos{static_cast<float>(x), static_cast<float>(y)};
  m_mouseDelta.x += newPos.x - m_mousePos.x;
  m_mouseDelta.y += newPos.y - m_mousePos.y;
  m_mousePos = newPos;
}

void Input::OnMouseButton(int button, int action, int mods) {
  (void)mods;
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return;
  }
  if (action == GLFW_PRESS) {
    m_mouseButtons[static_cast<size_t>(button)] = 1U;
    m_mousePressed[static_cast<size_t>(button)] = 1U;
  } else if (action == GLFW_RELEASE) {
    m_mouseButtons[static_cast<size_t>(button)] = 0U;
    m_mouseReleased[static_cast<size_t>(button)] = 1U;
  }
  if (m_actions) {
    m_actions->OnMouseButton(button, action);
  }
}

void Input::OnScroll(double xoff, double yoff) {
  m_scrollDelta.x += static_cast<float>(xoff);
  m_scrollDelta.y += static_cast<float>(yoff);
  if (m_actions) {
    m_actions->OnScroll(xoff, yoff);
  }
}

bool Input::IsKeyDown(int key) const {
#if defined(TE_DEBUG)
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
    assert(false && "Invalid GLFW key value.");
    return false;
  }
#else
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
    return false;
  }
#endif
  return m_keys[static_cast<size_t>(key)] != 0U;
}

bool Input::WasKeyPressed(int key) const {
#if defined(TE_DEBUG)
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
    assert(false && "Invalid GLFW key value.");
    return false;
  }
#else
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
    return false;
  }
#endif
  return m_keys[static_cast<size_t>(key)] != 0U && m_prevKeys[static_cast<size_t>(key)] == 0U;
}

bool Input::IsMouseDown(int button) const {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return false;
  }
  return m_mouseButtons[static_cast<size_t>(button)] != 0U;
}

bool Input::WasMousePressed(int button) const {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return false;
  }
  return m_mousePressed[static_cast<size_t>(button)] != 0U;
}

bool Input::WasMouseReleased(int button) const {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return false;
  }
  return m_mouseReleased[static_cast<size_t>(button)] != 0U;
}

} // namespace te
