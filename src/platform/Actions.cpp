#include "platform/Actions.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <cassert>

namespace te {

namespace {

struct KeyBinding {
  Action action;
  int key;
  bool requireCtrl;
};

struct MouseBinding {
  Action action;
  int button;
};

constexpr KeyBinding kKeyBindings[] = {
  {Action::MoveUp, GLFW_KEY_W, false},
  {Action::MoveDown, GLFW_KEY_S, false},
  {Action::MoveLeft, GLFW_KEY_A, false},
  {Action::MoveRight, GLFW_KEY_D, false},
  {Action::Save, GLFW_KEY_S, true},
  {Action::Load, GLFW_KEY_O, true},
  {Action::Undo, GLFW_KEY_Z, true},
  {Action::Redo, GLFW_KEY_Y, true},
  {Action::Quit, GLFW_KEY_Q, true},
  {Action::Tile1, GLFW_KEY_1, false},
  {Action::Tile2, GLFW_KEY_2, false},
  {Action::Tile3, GLFW_KEY_3, false},
  {Action::Tile4, GLFW_KEY_4, false},
  {Action::Tile5, GLFW_KEY_5, false},
  {Action::Tile6, GLFW_KEY_6, false},
  {Action::Tile7, GLFW_KEY_7, false},
  {Action::Tile8, GLFW_KEY_8, false},
  {Action::Tile9, GLFW_KEY_9, false},
};

constexpr MouseBinding kMouseBindings[] = {
  {Action::Paint, GLFW_MOUSE_BUTTON_LEFT},
  {Action::Erase, GLFW_MOUSE_BUTTON_RIGHT},
  {Action::PanDrag, GLFW_MOUSE_BUTTON_MIDDLE},
};

void ApplyDigital(ActionState& state, int glfwAction) {
  if (glfwAction == GLFW_PRESS) {
    state.down = true;
    state.pressed = true;
  } else if (glfwAction == GLFW_RELEASE) {
    state.down = false;
    state.released = true;
  } else if (glfwAction == GLFW_REPEAT) {
    state.down = true;
  }
}

bool IsCtrlKey(int key) {
  return key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL;
}

bool IsCtrlChordKey(int key) {
  return key == GLFW_KEY_S || key == GLFW_KEY_O || key == GLFW_KEY_Z || key == GLFW_KEY_Y || key == GLFW_KEY_Q;
}

} // namespace

void Actions::BeginFrame() {
  for (auto& state : m_states) {
    state.pressed = false;
    state.released = false;
    state.value = 0.0f;
  }
}

void Actions::OnKey(int glfwKey, int action) {
  if (glfwKey < GLFW_KEY_SPACE || glfwKey > GLFW_KEY_LAST) {
#if defined(TE_DEBUG)
    assert(false && "Invalid GLFW key value.");
#endif
    return;
  }

  if (IsCtrlKey(glfwKey)) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
      m_ctrlDown = true;
    } else if (action == GLFW_RELEASE) {
      m_ctrlDown = false;
    }
  }

  const bool ctrlChord = m_ctrlDown && IsCtrlChordKey(glfwKey);
  for (const auto& binding : kKeyBindings) {
    if (binding.key != glfwKey) {
      continue;
    }
    if (binding.requireCtrl) {
      if (!m_ctrlDown) {
        continue;
      }
    } else if (ctrlChord) {
      continue;
    }
    ApplyDigital(m_states[static_cast<size_t>(binding.action)], action);
  }
}

void Actions::OnMouseButton(int button, int action) {
  if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
#if defined(TE_DEBUG)
    assert(false && "Invalid GLFW mouse button value.");
#endif
    return;
  }

  for (const auto& binding : kMouseBindings) {
    if (binding.button != button) {
      continue;
    }
    ApplyDigital(m_states[static_cast<size_t>(binding.action)], action);
  }
}

void Actions::OnScroll(double xoff, double yoff) {
  (void)xoff;
  if (yoff > 0.0) {
    ActionState& zoomIn = m_states[static_cast<size_t>(Action::ZoomIn)];
    zoomIn.value += static_cast<float>(yoff);
    zoomIn.pressed = true;
  } else if (yoff < 0.0) {
    ActionState& zoomOut = m_states[static_cast<size_t>(Action::ZoomOut)];
    zoomOut.value += static_cast<float>(-yoff);
    zoomOut.pressed = true;
  }
}

const ActionState& Actions::Get(Action a) const {
  return m_states[static_cast<size_t>(a)];
}

} // namespace te
