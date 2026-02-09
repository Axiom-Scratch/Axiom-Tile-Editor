#include "editor/Tools.h"

#include "util/JsonLite.h"

#include <cmath>
#include <utility>

namespace te {

namespace {

Vec2i WorldToCell(const TileMap& map, const Vec2& world) {
  const int tileSize = map.GetTileSize();
  if (tileSize <= 0) {
    return {-1, -1};
  }

  const int x = static_cast<int>(std::floor(world.x / static_cast<float>(tileSize)));
  const int y = static_cast<int>(std::floor(world.y / static_cast<float>(tileSize)));
  return {x, y};
}

void BeginStroke(EditorState& state, StrokeButton button, int tileId) {
  state.strokeButton = button;
  state.strokeTileId = tileId;
  state.currentStroke.changes.clear();
}

void ApplyPaint(EditorState& state, int cellX, int cellY) {
  if (!state.tileMap.IsInBounds(cellX, cellY)) {
    return;
  }

  const int index = state.tileMap.Index(cellX, cellY);
  const int before = state.tileMap.GetTile(cellX, cellY);
  const int after = state.strokeTileId;
  if (before == after) {
    return;
  }

  state.tileMap.SetTile(cellX, cellY, after);
  AddOrUpdateChange(state.currentStroke, index, before, after);
}

} // namespace

void InitEditor(EditorState& state, int width, int height, int tileSize) {
  state.tileMap.Resize(width, height, tileSize);
  state.currentTileId = 1;
  state.strokeButton = StrokeButton::None;
  state.strokeTileId = 0;
  state.currentStroke.changes.clear();
  state.history.Clear();
}

void UpdateEditor(EditorState& state, const EditorInput& input) {
  if (input.tileSelect >= 1 && input.tileSelect <= 9) {
    state.currentTileId = input.tileSelect;
  }

  const Vec2i cell = WorldToCell(state.tileMap, input.mouseWorld);
  state.selection.hasHover = state.tileMap.IsInBounds(cell.x, cell.y);
  state.selection.hoverCell = cell;

  if (state.strokeButton == StrokeButton::None) {
    if (input.leftPressed) {
      BeginStroke(state, StrokeButton::Left, state.currentTileId);
    } else if (input.rightPressed) {
      BeginStroke(state, StrokeButton::Right, 0);
    }
  }

  if (state.strokeButton == StrokeButton::Left && input.leftDown && state.selection.hasHover) {
    ApplyPaint(state, cell.x, cell.y);
  }

  if (state.strokeButton == StrokeButton::Right && input.rightDown && state.selection.hasHover) {
    ApplyPaint(state, cell.x, cell.y);
  }

  if (state.strokeButton == StrokeButton::Left && input.leftReleased) {
    EndStroke(state);
  }

  if (state.strokeButton == StrokeButton::Right && input.rightReleased) {
    EndStroke(state);
  }
}

void EndStroke(EditorState& state) {
  if (state.strokeButton == StrokeButton::None) {
    return;
  }

  state.history.Push(std::move(state.currentStroke));
  state.currentStroke = PaintCommand{};
  state.strokeButton = StrokeButton::None;
  state.strokeTileId = 0;
}

bool SaveTileMap(const EditorState& state, const std::string& path) {
  return JsonLite::WriteTileMap(path, state.tileMap.GetWidth(), state.tileMap.GetHeight(),
                                state.tileMap.GetTileSize(), state.tileMap.GetData());
}

bool LoadTileMap(EditorState& state, const std::string& path, std::string* errorOut) {
  int width = 0;
  int height = 0;
  int tileSize = 0;
  std::vector<int> data;
  if (!JsonLite::ReadTileMap(path, width, height, tileSize, data, errorOut)) {
    return false;
  }

  state.tileMap.SetData(width, height, tileSize, std::move(data));
  state.history.Clear();
  return true;
}

bool Undo(EditorState& state) {
  return state.history.Undo(state.tileMap);
}

bool Redo(EditorState& state) {
  return state.history.Redo(state.tileMap);
}

} // namespace te
