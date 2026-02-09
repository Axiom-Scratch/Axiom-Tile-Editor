#pragma once

#include <vector>

namespace te {

class TileMap;

struct CellChange {
  int index = 0;
  int before = 0;
  int after = 0;
};

struct PaintCommand {
  std::vector<CellChange> changes;
};

class CommandHistory {
public:
  void Push(PaintCommand command);
  bool CanUndo() const { return !m_undo.empty(); }
  bool CanRedo() const { return !m_redo.empty(); }

  bool Undo(TileMap& map);
  bool Redo(TileMap& map);
  void Clear();

private:
  std::vector<PaintCommand> m_undo;
  std::vector<PaintCommand> m_redo;
};

void AddOrUpdateChange(PaintCommand& command, int index, int before, int after);

} // namespace te
