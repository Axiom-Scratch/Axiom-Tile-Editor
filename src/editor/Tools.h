#pragma once

#include "app/Config.h"

#include "editor/Atlas.h"
#include "editor/Commands.h"
#include "editor/Selection.h"
#include "editor/TileMap.h"

#include <string>
#include <vector>

namespace te {

enum class StrokeButton {
  None,
  Left,
  Right
};

enum class Tool {
  Paint,
  Erase,
  Rect,
  Fill,
  Pan
};

struct Layer {
  std::string name;
  bool visible = true;
  bool locked = false;
  float opacity = 1.0f;
};

struct EditorInput {
  Vec2 mouseWorld{};
  bool leftDown = false;
  bool rightDown = false;
  bool leftPressed = false;
  bool rightPressed = false;
  bool leftReleased = false;
  bool rightReleased = false;
  bool shift = false;
  bool ctrl = false;
  int tileSelect = 0;
};

struct EditorState {
  TileMap tileMap;
  Selection selection;
  CommandHistory history;
  Atlas atlas;

  int currentTileIndex = 1;
  Tool currentTool = Tool::Paint;
  std::vector<Layer> layers;
  int selectedLayer = -1;
  bool hasUnsavedChanges = false;
  StrokeButton strokeButton = StrokeButton::None;
  int strokeTileId = 0;
  PaintCommand currentStroke;

  bool rectActive = false;
  Vec2i rectStart{};
  Vec2i rectEnd{};
  bool rectErase = false;
};

void InitEditor(EditorState& state, int width, int height, int tileSize);
void UpdateEditor(EditorState& state, const EditorInput& input);
void EndStroke(EditorState& state);

bool SaveTileMap(const EditorState& state, const std::string& path);
bool LoadTileMap(EditorState& state, const std::string& path, std::string* errorOut = nullptr);

bool Undo(EditorState& state);
bool Redo(EditorState& state);

} // namespace te
