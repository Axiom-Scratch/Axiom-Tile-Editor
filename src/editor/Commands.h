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
  int mapWidth = 0;
  std::vector<CellChange> changes;
};

struct ResizeCommand {
  int oldWidth = 0;
  int oldHeight = 0;
  int newWidth = 0;
  int newHeight = 0;
  std::vector<std::vector<int>> beforeLayers;
  std::vector<std::vector<int>> afterLayers;
};

enum class CommandType {
  Paint,
  Resize
};

struct CommandEntry {
  CommandType type = CommandType::Paint;
  PaintCommand paint;
  ResizeCommand resize;
};

class CommandHistory {
public:
  void Push(PaintCommand command);
  void PushResize(ResizeCommand command);
  bool CanUndo() const { return !m_undo.empty(); }
  bool CanRedo() const { return !m_redo.empty(); }

  using ApplyChangeFn = std::function<void(int layerIndex, int x, int y, int value)>;
  using ApplyResizeFn = std::function<void(const ResizeCommand& command, bool redo)>;

  bool Undo(const ApplyChangeFn& apply, const ApplyResizeFn& resize);
  bool Redo(const ApplyChangeFn& apply, const ApplyResizeFn& resize);
  void Clear();

private:
  std::vector<CommandEntry> m_undo;
  std::vector<CommandEntry> m_redo;
};

void AddOrUpdateChange(PaintCommand& command, int index, int before, int after);

} // namespace te
