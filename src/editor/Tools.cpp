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

int ActiveLayerIndex(const EditorState& state) {
  if (state.activeLayer >= 0 && state.activeLayer < static_cast<int>(state.layers.size())) {
    return state.activeLayer;
  }
  return 0;
}

Layer& GetLayer(EditorState& state, int layerIndex) {
  return state.layers[static_cast<size_t>(layerIndex)];
}

const Layer& GetLayer(const EditorState& state, int layerIndex) {
  return state.layers[static_cast<size_t>(layerIndex)];
}

bool IsLayerLocked(const EditorState& state, int layerIndex) {
  if (layerIndex < 0 || layerIndex >= static_cast<int>(state.layers.size())) {
    return true;
  }
  return state.layers[static_cast<size_t>(layerIndex)].locked;
}

std::vector<int> ResizeLayerTiles(const std::vector<int>& source, int oldWidth, int oldHeight,
                                  int newWidth, int newHeight) {
  if (newWidth <= 0 || newHeight <= 0) {
    return {};
  }
  std::vector<int> result(static_cast<size_t>(newWidth * newHeight), 0);
  const int copyWidth = std::min(oldWidth, newWidth);
  const int copyHeight = std::min(oldHeight, newHeight);
  if (copyWidth <= 0 || copyHeight <= 0) {
    return result;
  }
  for (int y = 0; y < copyHeight; ++y) {
    for (int x = 0; x < copyWidth; ++x) {
      const int oldIndex = y * oldWidth + x;
      if (oldIndex < 0 || oldIndex >= static_cast<int>(source.size())) {
        continue;
      }
      const int newIndex = y * newWidth + x;
      result[static_cast<size_t>(newIndex)] = source[static_cast<size_t>(oldIndex)];
    }
  }
  return result;
}

int GetTileAt(const EditorState& state, int layerIndex, int x, int y) {
  if (!state.tileMap.IsInBounds(x, y)) {
    return 0;
  }
  const Layer& layer = GetLayer(state, layerIndex);
  const int index = state.tileMap.Index(x, y);
  if (index < 0 || index >= static_cast<int>(layer.tiles.size())) {
    return 0;
  }
  return layer.tiles[static_cast<size_t>(index)];
}

void SetTileAt(EditorState& state, int layerIndex, int x, int y, int value) {
  if (!state.tileMap.IsInBounds(x, y)) {
    return;
  }
  Layer& layer = GetLayer(state, layerIndex);
  const int index = state.tileMap.Index(x, y);
  if (index < 0) {
    return;
  }
  if (index >= static_cast<int>(layer.tiles.size())) {
    layer.tiles.resize(static_cast<size_t>(state.tileMap.GetWidth() * state.tileMap.GetHeight()), 0);
  }
  layer.tiles[static_cast<size_t>(index)] = value;
}

void BeginStroke(EditorState& state, StrokeButton button, int tileId) {
  state.strokeButton = button;
  state.strokeTileId = tileId;
  state.currentStroke.layerIndex = ActiveLayerIndex(state);
  state.currentStroke.mapWidth = state.tileMap.GetWidth();
  state.currentStroke.changes.clear();
}

void ApplyBrush(EditorState& state, int layerIndex, int cellX, int cellY, int tileId, PaintCommand& command) {
  const int size = std::max(1, state.brushSize);
  const int half = size / 2;
  const int startX = cellX - half;
  const int startY = cellY - half;
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      const int cx = startX + x;
      const int cy = startY + y;
      if (!state.tileMap.IsInBounds(cx, cy)) {
        continue;
      }

      const int index = state.tileMap.Index(cx, cy);
      const int before = GetTileAt(state, layerIndex, cx, cy);
      const int after = tileId;
      if (before == after) {
        continue;
      }

      SetTileAt(state, layerIndex, cx, cy, after);
      AddOrUpdateChange(command, index, before, after);
      state.hasUnsavedChanges = true;
    }
  }
}

void ApplyPaint(EditorState& state, int cellX, int cellY) {
  ApplyBrush(state, ActiveLayerIndex(state), cellX, cellY, state.strokeTileId, state.currentStroke);
  state.hasLastPaintCell = true;
  state.lastPaintCell = {cellX, cellY};
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
  command.layerIndex = ActiveLayerIndex(state);
  command.mapWidth = state.tileMap.GetWidth();

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
      const int before = GetTileAt(state, command.layerIndex, x, y);
      if (before == tileId) {
        continue;
      }
      SetTileAt(state, command.layerIndex, x, y, tileId);
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

  const int layerIndex = ActiveLayerIndex(state);
  const int target = GetTileAt(state, layerIndex, startX, startY);
  if (target == tileId) {
    return;
  }

  const int width = state.tileMap.GetWidth();
  const int height = state.tileMap.GetHeight();
  std::vector<unsigned char> visited(static_cast<size_t>(width * height), 0U);
  std::vector<Vec2i> stack;
  stack.push_back({startX, startY});

  PaintCommand command;
  command.layerIndex = layerIndex;
  command.mapWidth = state.tileMap.GetWidth();
  command.mapWidth = state.tileMap.GetWidth();

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

    if (GetTileAt(state, layerIndex, cell.x, cell.y) != target) {
      continue;
    }

    SetTileAt(state, layerIndex, cell.x, cell.y, tileId);
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
  command.layerIndex = ActiveLayerIndex(state);
  command.mapWidth = state.tileMap.GetWidth();

  for (const Vec2i& cell : cells) {
    if (!state.tileMap.IsInBounds(cell.x, cell.y)) {
      continue;
    }
    ApplyBrush(state, command.layerIndex, cell.x, cell.y, tileId, command);
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

  const int layerIndex = ActiveLayerIndex(state);
  if (IsLayerLocked(state, layerIndex)) {
    return;
  }
  const int width = state.tileMap.GetWidth();
  const int height = state.tileMap.GetHeight();
  std::vector<int> original = GetLayer(state, layerIndex).tiles;
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
  command.layerIndex = layerIndex;
  for (int index : affected) {
    const int before = original[static_cast<size_t>(index)];
    const int after = updated[static_cast<size_t>(index)];
    if (before == after) {
      continue;
    }
    const int x = index % width;
    const int y = index / width;
    SetTileAt(state, layerIndex, x, y, after);
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
  Layer baseLayer{};
  baseLayer.name = "Layer 0";
  baseLayer.visible = true;
  baseLayer.locked = false;
  baseLayer.opacity = 1.0f;
  baseLayer.tiles.assign(static_cast<size_t>(width * height), 0);
  state.layers.push_back(std::move(baseLayer));
  state.selectedLayer = -1;
  state.activeLayer = 0;
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
  state.mouseWorld = {};
  state.brushSize = 1;
  state.previousTool = Tool::Paint;
  state.hasLastPaintCell = false;
  state.lastPaintCell = {};
  state.stampWidth = 0;
  state.stampHeight = 0;
  state.stampTiles.clear();
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

  state.mouseWorld = input.mouseWorld;
  const Vec2i cell = WorldToCell(state.tileMap, input.mouseWorld);
  state.selection.hasHover = state.tileMap.IsInBounds(cell.x, cell.y);
  state.selection.hoverCell = cell;

  if (state.currentTool == Tool::Pick) {
    if (input.leftPressed && state.selection.hasHover) {
      int picked = 0;
      for (int i = static_cast<int>(state.layers.size()) - 1; i >= 0; --i) {
        if (!state.layers[static_cast<size_t>(i)].visible) {
          continue;
        }
        const int value = GetTileAt(state, i, cell.x, cell.y);
        if (value != 0) {
          picked = value;
          break;
        }
      }
      state.currentTileIndex = picked;
      state.currentTool = state.previousTool;
    }
    return;
  }

  const bool selectMode = state.currentTool == Tool::Select;
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

  const int layerIndex = ActiveLayerIndex(state);
  const bool layerLocked = IsLayerLocked(state, layerIndex);

  if (!selectMode && !layerLocked && state.currentTool == Tool::Fill && input.leftPressed && state.selection.hasHover) {
    FloodFill(state, cell.x, cell.y, state.currentTileIndex);
  }

  if (!selectMode && !layerLocked && state.currentTool == Tool::Rect) {
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
  } else if (!selectMode && !layerLocked && state.currentTool == Tool::Line) {
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
  } else if (!selectMode && !layerLocked && state.currentTool == Tool::Move) {
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
  } else if (!selectMode && !layerLocked && state.currentTool == Tool::Stamp) {
    if (input.leftPressed && state.selection.hasHover && state.stampWidth > 0 && state.stampHeight > 0 &&
        !state.stampTiles.empty()) {
      PaintCommand command;
      command.layerIndex = layerIndex;
      command.mapWidth = state.tileMap.GetWidth();
      const int startX = cell.x;
      const int startY = cell.y;
      for (int y = 0; y < state.stampHeight; ++y) {
        for (int x = 0; x < state.stampWidth; ++x) {
          const int sx = startX + x;
          const int sy = startY + y;
          if (!state.tileMap.IsInBounds(sx, sy)) {
            continue;
          }
          const int stampIndex = y * state.stampWidth + x;
          if (stampIndex < 0 || stampIndex >= static_cast<int>(state.stampTiles.size())) {
            continue;
          }
          const int tileId = state.stampTiles[static_cast<size_t>(stampIndex)];
          const int index = state.tileMap.Index(sx, sy);
          const int before = GetTileAt(state, layerIndex, sx, sy);
          if (before == tileId) {
            continue;
          }
          SetTileAt(state, layerIndex, sx, sy, tileId);
          AddOrUpdateChange(command, index, before, tileId);
        }
      }
      if (!command.changes.empty()) {
        state.history.Push(std::move(command));
        state.hasUnsavedChanges = true;
      }
    }
  } else if (!selectMode && !layerLocked) {
    if (input.shift && input.leftPressed && state.selection.hasHover && state.hasLastPaintCell &&
        (state.currentTool == Tool::Paint || state.currentTool == Tool::Erase)) {
      const int tileId = (state.currentTool == Tool::Erase) ? 0 : state.currentTileIndex;
      ApplyLine(state, state.lastPaintCell, cell, tileId);
      state.hasLastPaintCell = true;
      state.lastPaintCell = cell;
      return;
    }

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

void ApplyResizeCommand(EditorState& state, const ResizeCommand& command, bool redo) {
  const int targetWidth = redo ? command.newWidth : command.oldWidth;
  const int targetHeight = redo ? command.newHeight : command.oldHeight;
  if (targetWidth <= 0 || targetHeight <= 0) {
    return;
  }
  const int currentWidth = state.tileMap.GetWidth();
  const int currentHeight = state.tileMap.GetHeight();

  state.tileMap.Resize(targetWidth, targetHeight, state.tileMap.GetTileSize());

  const auto& layerData = redo ? command.afterLayers : command.beforeLayers;
  for (size_t i = 0; i < state.layers.size(); ++i) {
    if (i < layerData.size()) {
      state.layers[i].tiles = layerData[i];
    } else {
      state.layers[i].tiles =
          ResizeLayerTiles(state.layers[i].tiles, currentWidth, currentHeight, targetWidth, targetHeight);
    }
    if (static_cast<int>(state.layers[i].tiles.size()) != targetWidth * targetHeight) {
      state.layers[i].tiles.assign(static_cast<size_t>(targetWidth * targetHeight), 0);
    }
  }
  state.selection.Resize(targetWidth, targetHeight);
  state.rectActive = false;
  state.lineActive = false;
  state.moveActive = false;
  state.hasLastPaintCell = false;
}

bool SetMapSize(EditorState& state, int width, int height) {
  if (width <= 0 || height <= 0) {
    return false;
  }
  const int oldWidth = state.tileMap.GetWidth();
  const int oldHeight = state.tileMap.GetHeight();
  if (width == oldWidth && height == oldHeight) {
    return false;
  }

  ResizeCommand command;
  command.oldWidth = oldWidth;
  command.oldHeight = oldHeight;
  command.newWidth = width;
  command.newHeight = height;
  command.beforeLayers.reserve(state.layers.size());
  command.afterLayers.reserve(state.layers.size());

  for (Layer& layer : state.layers) {
    command.beforeLayers.push_back(layer.tiles);
    std::vector<int> resized = ResizeLayerTiles(layer.tiles, oldWidth, oldHeight, width, height);
    command.afterLayers.push_back(resized);
    layer.tiles = std::move(resized);
  }

  state.tileMap.Resize(width, height, state.tileMap.GetTileSize());
  state.selection.Resize(width, height);
  state.rectActive = false;
  state.lineActive = false;
  state.moveActive = false;
  state.hasLastPaintCell = false;
  state.history.PushResize(std::move(command));
  state.hasUnsavedChanges = true;
  return true;
}

bool SaveTileMap(const EditorState& state, const std::string& path) {
  std::vector<JsonLite::LayerInfo> layers;
  layers.reserve(state.layers.size());
  for (const Layer& layer : state.layers) {
    JsonLite::LayerInfo info;
    info.name = layer.name;
    info.visible = layer.visible;
    info.locked = layer.locked;
    info.opacity = layer.opacity;
    info.data = layer.tiles;
    layers.push_back(std::move(info));
  }
  return JsonLite::WriteTileMap(path, state.tileMap.GetWidth(), state.tileMap.GetHeight(),
                                state.tileMap.GetTileSize(), state.atlas, layers);
}

bool LoadTileMap(EditorState& state, const std::string& path, std::string* errorOut) {
  int width = 0;
  int height = 0;
  int tileSize = 0;
  std::vector<JsonLite::LayerInfo> layers;
  Atlas loadedAtlas;
  if (!JsonLite::ReadTileMap(path, width, height, tileSize, loadedAtlas, state.atlas, layers, errorOut)) {
    return false;
  }

  state.tileMap.Resize(width, height, tileSize);
  state.atlas = loadedAtlas;
  state.layers.clear();
  state.layers.reserve(layers.size());
  for (const JsonLite::LayerInfo& info : layers) {
    Layer layer;
    layer.name = info.name;
    layer.visible = info.visible;
    layer.locked = info.locked;
    layer.opacity = info.opacity;
    layer.tiles = info.data;
    if (static_cast<int>(layer.tiles.size()) != width * height) {
      layer.tiles.assign(static_cast<size_t>(width * height), 0);
    }
    state.layers.push_back(std::move(layer));
  }
  if (state.layers.empty()) {
    Layer layer;
    layer.name = "Layer 0";
    layer.tiles.assign(static_cast<size_t>(width * height), 0);
    state.layers.push_back(std::move(layer));
  }
  state.activeLayer = 0;
  state.selectedLayer = -1;
  state.history.Clear();
  state.selection.Resize(width, height);
  state.rectActive = false;
  state.lineActive = false;
  state.moveActive = false;
  state.hasLastPaintCell = false;
  state.hasUnsavedChanges = false;
  return true;
}

bool Undo(EditorState& state) {
  const bool result = state.history.Undo(
      [&](int layerIndex, int x, int y, int value) {
        if (layerIndex < 0 || layerIndex >= static_cast<int>(state.layers.size())) {
          return;
        }
        SetTileAt(state, layerIndex, x, y, value);
      },
      [&](const ResizeCommand& command, bool redo) { ApplyResizeCommand(state, command, redo); });
  if (result) {
    state.hasUnsavedChanges = true;
  }
  return result;
}

bool Redo(EditorState& state) {
  const bool result = state.history.Redo(
      [&](int layerIndex, int x, int y, int value) {
        if (layerIndex < 0 || layerIndex >= static_cast<int>(state.layers.size())) {
          return;
        }
        SetTileAt(state, layerIndex, x, y, value);
      },
      [&](const ResizeCommand& command, bool redo) { ApplyResizeCommand(state, command, redo); });
  if (result) {
    state.hasUnsavedChanges = true;
  }
  return result;
}

} // namespace te
