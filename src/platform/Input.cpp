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
  if (!s_instance) {
    return;
  }
  if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST) {
#if defined(TE_DEBUG)
    assert(false && "Invalid GLFW key value.");
#endif
    return;
  }

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    s_instance->m_keys[static_cast<size_t>(key)] = 1U;
  } else if (action == GLFW_RELEASE) {
    s_instance->m_keys[static_cast<size_t>(key)] = 0U;
  }

  if (s_instance->m_actions) {
    s_instance->m_actions->OnKey(key, action);
  }
}

void Input::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  (void)window;
  (void)mods;
  if (!s_instance) {
    return;
  }
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return;
  }
  if (s_instance->m_actions) {
    s_instance->m_actions->OnMouseButton(button, action);
  }
}

void Input::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  (void)window;
  if (s_instance) {
    s_instance->m_scrollDelta.x += static_cast<float>(xoffset);
    s_instance->m_scrollDelta.y += static_cast<float>(yoffset);
    if (s_instance->m_actions) {
      s_instance->m_actions->OnScroll(xoffset, yoffset);
    }
  }
}

void Input::Attach(GLFWwindow* window) {
  m_window = window;
  s_instance = this;
  if (m_window) {
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);
  }
}

void Input::BeginFrame() {
  m_prevKeys = m_keys;
  m_scrollDelta = {};
}

void Input::Update(GLFWwindow* window) {
  if (!window) {
    return;
  }

  m_prevMouseButtons = m_mouseButtons;
  m_prevMousePos = m_mousePos;

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
  return m_mouseButtons[static_cast<size_t>(button)] != 0U && m_prevMouseButtons[static_cast<size_t>(button)] == 0U;
}

bool Input::WasMouseReleased(int button) const {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
    return false;
  }
  return m_mouseButtons[static_cast<size_t>(button)] == 0U && m_prevMouseButtons[static_cast<size_t>(button)] != 0U;
}

} // namespace te
