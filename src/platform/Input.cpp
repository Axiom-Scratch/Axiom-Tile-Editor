#include "platform/Input.h"

namespace te {

Input* Input::s_instance = nullptr;

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)window;
  if (s_instance) {
    s_instance->m_scrollDelta.x += static_cast<float>(xoffset);
    s_instance->m_scrollDelta.y += static_cast<float>(yoffset);
  }
}

void Input::Attach(GLFWwindow* window) {
  m_window = window;
  s_instance = this;
  if (m_window) {
    glfwSetScrollCallback(m_window, ScrollCallback);
  }
}

void Input::BeginFrame() {
  m_scrollDelta = {};
}

void Input::Update(GLFWwindow* window) {
  if (!window) {
    return;
  }

  m_prevKeys = m_keys;
  m_prevMouseButtons = m_mouseButtons;
  m_prevMousePos = m_mousePos;

  for (int key = 0; key <= GLFW_KEY_LAST; ++key) {
    m_keys[static_cast<size_t>(key)] = static_cast<unsigned char>(glfwGetKey(window, key) == GLFW_PRESS);
  }

  for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
    m_mouseButtons[static_cast<size_t>(button)] =
        static_cast<unsigned char>(glfwGetMouseButton(window, button) == GLFW_PRESS);
  }

  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(window, &x, &y);
  m_mousePos = {static_cast<float>(x), static_cast<float>(y)};
}

bool Input::IsKeyDown(int key) const {
  if (key < 0 || key > GLFW_KEY_LAST) {
    return false;
  }
  return m_keys[static_cast<size_t>(key)] != 0U;
}

bool Input::WasKeyPressed(int key) const {
  if (key < 0 || key > GLFW_KEY_LAST) {
    return false;
  }
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
  return m_mouseButtons[static_cast<size_t>(button)] != 0U && m_prevMouseButtons[static_cast<size_t>(button)] == 0U;
}

bool Input::WasMouseReleased(int button) const {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return false;
  }
  return m_mouseButtons[static_cast<size_t>(button)] == 0U && m_prevMouseButtons[static_cast<size_t>(button)] != 0U;
}

} // namespace te
