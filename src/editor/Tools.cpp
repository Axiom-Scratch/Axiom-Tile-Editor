#include "editor/Tools.h"

#include "util/JsonLite.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

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
  state.hasUnsavedChanges = true;
}

SelectionMode GetSelectionMode(const EditorInput& input) {
  if (input.ctrl) {
    return SelectionMode::Toggle;
  }
  if (input.shift) {
    return SelectionMode::Add;
  }
  return SelectionMode::Replace;
}

void ApplyRect(EditorState& state, const Vec2i& a, const Vec2i& b, int tileId) {
  PaintCommand command;

  int minX = std::min(a.x, b.x);
  int maxX = std::max(a.x, b.x);
  int minY = std::min(a.y, b.y);
  int maxY = std::max(a.y, b.y);

  minX = std::max(0, minX);
  minY = std::max(0, minY);
  maxX = std::min(state.tileMap.GetWidth() - 1, maxX);
  maxY = std::min(state.tileMap.GetHeight() - 1, maxY);

  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      const int index = state.tileMap.Index(x, y);
      const int before = state.tileMap.GetTile(x, y);
      if (before == tileId) {
        continue;
      }
      state.tileMap.SetTile(x, y, tileId);
      AddOrUpdateChange(command, index, before, tileId);
    }
  }

  if (!command.changes.empty()) {
    state.history.Push(std::move(command));
    state.hasUnsavedChanges = true;
  }
}

void FloodFill(EditorState& state, int startX, int startY, int tileId) {
  if (!state.tileMap.IsInBounds(startX, startY)) {
    return;
  }

  const int target = state.tileMap.GetTile(startX, startY);
  if (target == tileId) {
    return;
  }

  const int width = state.tileMap.GetWidth();
  const int height = state.tileMap.GetHeight();
  std::vector<unsigned char> visited(static_cast<size_t>(width * height), 0U);
  std::vector<Vec2i> stack;
  stack.push_back({startX, startY});

  PaintCommand command;

  while (!stack.empty()) {
    Vec2i cell = stack.back();
    stack.pop_back();

    if (!state.tileMap.IsInBounds(cell.x, cell.y)) {
      continue;
    }
    const int index = state.tileMap.Index(cell.x, cell.y);
    if (visited[static_cast<size_t>(index)] != 0U) {
      continue;
    }
    visited[static_cast<size_t>(index)] = 1U;

    if (state.tileMap.GetTile(cell.x, cell.y) != target) {
      continue;
    }

    state.tileMap.SetTile(cell.x, cell.y, tileId);
    AddOrUpdateChange(command, index, target, tileId);

    stack.push_back({cell.x + 1, cell.y});
    stack.push_back({cell.x - 1, cell.y});
    stack.push_back({cell.x, cell.y + 1});
    stack.push_back({cell.x, cell.y - 1});
  }

  if (!command.changes.empty()) {
    state.history.Push(std::move(command));
    state.hasUnsavedChanges = true;
  }
}

void ApplyLine(EditorState& state, const Vec2i& a, const Vec2i& b, int tileId) {
  std::vector<Vec2i> cells;
  BuildLineCells(a, b, cells);
  PaintCommand command;

  for (const Vec2i& cell : cells) {
    if (!state.tileMap.IsInBounds(cell.x, cell.y)) {
      continue;
    }
    const int index = state.tileMap.Index(cell.x, cell.y);
    const int before = state.tileMap.GetTile(cell.x, cell.y);
    if (before == tileId) {
      continue;
    }
    state.tileMap.SetTile(cell.x, cell.y, tileId);
    AddOrUpdateChange(command, index, before, tileId);
  }

  if (!command.changes.empty()) {
    state.history.Push(std::move(command));
    state.hasUnsavedChanges = true;
  }
}

void ApplyMoveSelection(EditorState& state, const Vec2i& delta) {
  if (delta.x == 0 && delta.y == 0) {
    return;
  }
  if (!state.selection.HasSelection()) {
    return;
  }

  const int width = state.tileMap.GetWidth();
  const int height = state.tileMap.GetHeight();
  std::vector<int> original = state.tileMap.GetData();
  std::vector<int> updated = original;
  std::vector<int> affected;
  const std::vector<int> selected = state.selection.indices;
  affected.reserve(selected.size() * 2);

  for (int index : selected) {
    if (index < 0 || index >= width * height) {
      continue;
    }
    updated[static_cast<size_t>(index)] = 0;
    affected.push_back(index);
  }

  for (int index : selected) {
    if (index < 0 || index >= width * height) {
      continue;
    }
    const int x = index % width;
    const int y = index / width;
    const int destX = x + delta.x;
    const int destY = y + delta.y;
    if (destX < 0 || destY < 0 || destX >= width || destY >= height) {
      continue;
    }
    const int destIndex = destY * width + destX;
    updated[static_cast<size_t>(destIndex)] = original[static_cast<size_t>(index)];
    affected.push_back(destIndex);
  }

  std::sort(affected.begin(), affected.end());
  affected.erase(std::unique(affected.begin(), affected.end()), affected.end());

  PaintCommand command;
  for (int index : affected) {
    const int before = original[static_cast<size_t>(index)];
    const int after = updated[static_cast<size_t>(index)];
    if (before == after) {
      continue;
    }
    const int x = index % width;
    const int y = index / width;
    state.tileMap.SetTile(x, y, after);
    AddOrUpdateChange(command, index, before, after);
  }

  if (!command.changes.empty()) {
    state.history.Push(std::move(command));
    state.hasUnsavedChanges = true;
  }

  state.selection.Clear();
  for (int index : selected) {
    if (index < 0 || index >= width * height) {
      continue;
    }
    const int x = index % width;
    const int y = index / width;
    const int destX = x + delta.x;
    const int destY = y + delta.y;
    if (destX < 0 || destY < 0 || destX >= width || destY >= height) {
      continue;
    }
    const int destIndex = destY * width + destX;
    state.selection.SetSelected(destIndex, true);
  }
}

} // namespace

void InitEditor(EditorState& state, int width, int height, int tileSize) {
  state.tileMap.Resize(width, height, tileSize);
  state.atlas.path = "assets/textures/atlas.png";
  state.atlas.tileW = tileSize;
  state.atlas.tileH = tileSize;
  state.atlas.cols = 0;
  state.atlas.rows = 0;
  state.currentTileIndex = 1;
  state.currentTool = Tool::Paint;
  state.layers.clear();
  state.layers.push_back({"Layer 0", true, false, 1.0f});
  state.selectedLayer = -1;
  state.selection.Resize(width, height);
  state.hasUnsavedChanges = false;
  state.strokeButton = StrokeButton::None;
  state.strokeTileId = 0;
  state.currentStroke.changes.clear();
  state.rectActive = false;
  state.rectStart = {};
  state.rectEnd = {};
  state.rectErase = false;
  state.lineActive = false;
  state.lineStart = {};
  state.lineEnd = {};
  state.moveActive = false;
  state.moveStart = {};
  state.moveEnd = {};
  state.sceneBgColor = {0.18f, 0.18f, 0.20f, 1.0f};
  state.history.Clear();
}

void BuildLineCells(const Vec2i& a, const Vec2i& b, std::vector<Vec2i>& out) {
  out.clear();
  int x0 = a.x;
  int y0 = a.y;
  int x1 = b.x;
  int y1 = b.y;
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    out.push_back({x0, y0});
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int e2 = err * 2;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void UpdateEditor(EditorState& state, const EditorInput& input) {
  if (input.tileSelect >= 1 && input.tileSelect <= 9) {
    state.currentTileIndex = input.tileSelect;
  }

  const Vec2i cell = WorldToCell(state.tileMap, input.mouseWorld);
  state.selection.hasHover = state.tileMap.IsInBounds(cell.x, cell.y);
  state.selection.hoverCell = cell;

  const bool selectMode = state.currentTool == Tool::Select || input.shift;
  if (selectMode) {
    if (!state.selection.isSelecting && input.leftPressed && state.selection.hasHover) {
      state.selection.BeginRect(cell);
    }
    if (state.selection.isSelecting && input.leftDown) {
      state.selection.UpdateRect(cell);
    }
    if (state.selection.isSelecting && input.leftReleased) {
      state.selection.EndRect(GetSelectionMode(input));
    }
  }

  if (!selectMode && state.currentTool == Tool::Fill && input.leftPressed && state.selection.hasHover) {
    FloodFill(state, cell.x, cell.y, state.currentTileIndex);
  }

  if (!selectMode && state.currentTool == Tool::Rect) {
    if (!state.rectActive && state.selection.hasHover) {
      if (input.leftPressed) {
        state.rectActive = true;
        state.rectStart = cell;
        state.rectEnd = cell;
        state.rectErase = false;
      } else if (input.rightPressed) {
        state.rectActive = true;
        state.rectStart = cell;
        state.rectEnd = cell;
        state.rectErase = true;
      }
    }

    if (state.rectActive && (input.leftDown || input.rightDown)) {
      state.rectEnd = cell;
    }

    if (state.rectActive && (input.leftReleased || input.rightReleased)) {
      const int tileId = state.rectErase ? 0 : state.currentTileIndex;
      ApplyRect(state, state.rectStart, state.rectEnd, tileId);
      state.rectActive = false;
    }
  } else if (!selectMode && state.currentTool == Tool::Line) {
    if (!state.lineActive && input.leftPressed && state.selection.hasHover) {
      state.lineActive = true;
      state.lineStart = cell;
      state.lineEnd = cell;
    }
    if (state.lineActive && input.leftDown) {
      state.lineEnd = cell;
    }
    if (state.lineActive && input.leftReleased) {
      ApplyLine(state, state.lineStart, state.lineEnd, state.currentTileIndex);
      state.lineActive = false;
    }
  } else if (!selectMode && state.currentTool == Tool::Move) {
    if (!state.moveActive && input.leftPressed && state.selection.hasHover) {
      const int index = state.tileMap.Index(cell.x, cell.y);
      if (state.selection.IsSelected(index)) {
        state.moveActive = true;
        state.moveStart = cell;
        state.moveEnd = cell;
      }
    }
    if (state.moveActive && input.leftDown) {
      state.moveEnd = cell;
    }
    if (state.moveActive && input.leftReleased) {
      const Vec2i delta{state.moveEnd.x - state.moveStart.x, state.moveEnd.y - state.moveStart.y};
      ApplyMoveSelection(state, delta);
      state.moveActive = false;
    }
  } else if (!selectMode) {
    if (state.strokeButton == StrokeButton::None) {
      if (input.leftPressed) {
        if (state.currentTool == Tool::Erase) {
          BeginStroke(state, StrokeButton::Left, 0);
        } else if (state.currentTool == Tool::Paint) {
          BeginStroke(state, StrokeButton::Left, state.currentTileIndex);
        }
      }
    }

    if (state.strokeButton == StrokeButton::Left && input.leftDown && state.selection.hasHover) {
      ApplyPaint(state, cell.x, cell.y);
    }

    if (state.strokeButton == StrokeButton::Left && input.leftReleased) {
      EndStroke(state);
    }
  }
}

void EndStroke(EditorState& state) {
  if (state.strokeButton == StrokeButton::None) {
    return;
  }

  if (!state.currentStroke.changes.empty()) {
    state.history.Push(std::move(state.currentStroke));
  }
  state.currentStroke = PaintCommand{};
  state.strokeButton = StrokeButton::None;
  state.strokeTileId = 0;
}

bool SaveTileMap(const EditorState& state, const std::string& path) {
  return JsonLite::WriteTileMap(path, state.tileMap.GetWidth(), state.tileMap.GetHeight(),
                                state.tileMap.GetTileSize(), state.atlas, state.tileMap.GetData());
}

bool LoadTileMap(EditorState& state, const std::string& path, std::string* errorOut) {
  int width = 0;
  int height = 0;
  int tileSize = 0;
  std::vector<int> data;
  Atlas loadedAtlas;
  if (!JsonLite::ReadTileMap(path, width, height, tileSize, loadedAtlas, state.atlas, data, errorOut)) {
    return false;
  }

  state.tileMap.SetData(width, height, tileSize, std::move(data));
  state.atlas = loadedAtlas;
  state.history.Clear();
  state.selection.Resize(width, height);
  state.rectActive = false;
  state.lineActive = false;
  state.moveActive = false;
  state.hasUnsavedChanges = false;
  return true;
}

bool Undo(EditorState& state) {
  const bool result = state.history.Undo(state.tileMap);
  if (result) {
    state.hasUnsavedChanges = true;
  }
  return result;
}

bool Redo(EditorState& state) {
  const bool result = state.history.Redo(state.tileMap);
  if (result) {
    state.hasUnsavedChanges = true;
  }
  return result;
}

} // namespace te
