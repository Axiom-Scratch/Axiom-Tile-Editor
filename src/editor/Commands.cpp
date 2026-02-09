#include "editor/Commands.h"

#include "editor/TileMap.h"

#include <utility>

namespace te {

void CommandHistory::Push(PaintCommand command) {
  if (command.changes.empty()) {
    return;
  }
  m_undo.push_back(std::move(command));
  m_redo.clear();
}

bool CommandHistory::Undo(TileMap& map) {
  if (m_undo.empty()) {
    return false;
  }

  PaintCommand command = std::move(m_undo.back());
  m_undo.pop_back();

  for (const CellChange& change : command.changes) {
    const int x = change.index % map.GetWidth();
    const int y = change.index / map.GetWidth();
    map.SetTile(x, y, change.before);
  }

  m_redo.push_back(std::move(command));
  return true;
}

bool CommandHistory::Redo(TileMap& map) {
  if (m_redo.empty()) {
    return false;
  }

  PaintCommand command = std::move(m_redo.back());
  m_redo.pop_back();

  for (const CellChange& change : command.changes) {
    const int x = change.index % map.GetWidth();
    const int y = change.index / map.GetWidth();
    map.SetTile(x, y, change.after);
  }

  m_undo.push_back(std::move(command));
  return true;
}

void CommandHistory::Clear() {
  m_undo.clear();
  m_redo.clear();
}

void AddOrUpdateChange(PaintCommand& command, int index, int before, int after) {
  for (CellChange& change : command.changes) {
    if (change.index == index) {
      change.after = after;
      return;
    }
  }
  command.changes.push_back({index, before, after});
}

} // namespace te
