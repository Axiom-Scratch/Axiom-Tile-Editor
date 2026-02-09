#pragma once

#include <array>
#include <cstddef>

namespace te {

enum class Action {
  MoveUp,
  MoveDown,
  MoveLeft,
  MoveRight,
  ZoomIn,
  ZoomOut,
  Paint,
  Erase,
  Save,
  Load,
  Undo,
  Redo,
  Tile1,
  Tile2,
  Tile3,
  Tile4,
  Tile5,
  Tile6,
  Tile7,
  Tile8,
  Tile9,
  PanDrag,
  Count
};

struct ActionState {
  bool down = false;
  bool pressed = false;
  bool released = false;
  float value = 0.0f;
};

class Actions {
public:
  Actions() = default;

  void BeginFrame();
  void OnKey(int glfwKey, int action);
  void OnMouseButton(int button, int action);
  void OnScroll(double xoff, double yoff);
  const ActionState& Get(Action a) const;

private:
  std::array<ActionState, static_cast<size_t>(Action::Count)> m_states{};
  bool m_ctrlDown = false;
};

} // namespace te
