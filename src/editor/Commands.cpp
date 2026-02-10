#include "editor/Commands.h"

#include <utility>

namespace te {

void CommandHistory::Push(PaintCommand command) {
  if (command.changes.empty()) {
    return;
  }
  CommandEntry entry;
  entry.type = CommandType::Paint;
  entry.paint = std::move(command);
  m_undo.push_back(std::move(entry));
  m_redo.clear();
}

void CommandHistory::PushResize(ResizeCommand command) {
  CommandEntry entry;
  entry.type = CommandType::Resize;
  entry.resize = std::move(command);
  m_undo.push_back(std::move(entry));
  m_redo.clear();
}

bool CommandHistory::Undo(const ApplyChangeFn& apply, const ApplyResizeFn& resize) {
  if (m_undo.empty()) {
    return false;
  }

  CommandEntry entry = std::move(m_undo.back());
  m_undo.pop_back();

  if (entry.type == CommandType::Paint) {
    const int width = entry.paint.mapWidth;
    for (const CellChange& change : entry.paint.changes) {
      const int x = change.index % width;
      const int y = change.index / width;
      apply(entry.paint.layerIndex, x, y, change.before);
    }
  } else {
    resize(entry.resize, false);
  }

  m_redo.push_back(std::move(entry));
  return true;
}

bool CommandHistory::Redo(const ApplyChangeFn& apply, const ApplyResizeFn& resize) {
  if (m_redo.empty()) {
    return false;
  }

  CommandEntry entry = std::move(m_redo.back());
  m_redo.pop_back();

  if (entry.type == CommandType::Paint) {
    const int width = entry.paint.mapWidth;
    for (const CellChange& change : entry.paint.changes) {
      const int x = change.index % width;
      const int y = change.index / width;
      apply(entry.paint.layerIndex, x, y, change.after);
    }
  } else {
    resize(entry.resize, true);
  }

  m_undo.push_back(std::move(entry));
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
