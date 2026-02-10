#pragma once

#include <functional>
#include <vector>

namespace te {

class TileMap;

struct CellChange {
  int index = 0;
  int before = 0;
  int after = 0;
};

struct PaintCommand {
  int layerIndex = 0;
  std::vector<CellChange> changes;
};

class CommandHistory {
public:
  void Push(PaintCommand command);
  bool CanUndo() const { return !m_undo.empty(); }
  bool CanRedo() const { return !m_redo.empty(); }

  using ApplyChangeFn = std::function<void(int layerIndex, int x, int y, int value)>;

  bool Undo(int width, const ApplyChangeFn& apply);
  bool Redo(int width, const ApplyChangeFn& apply);
  void Clear();

private:
  std::vector<PaintCommand> m_undo;
  std::vector<PaintCommand> m_redo;
};

void AddOrUpdateChange(PaintCommand& command, int index, int before, int after);

} // namespace te
