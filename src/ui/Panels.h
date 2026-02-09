#pragma once

#include "app/Config.h"
#include "editor/Tools.h"
#include "render/OrthoCamera.h"
#include "util/Log.h"

#include <string>
#include <vector>

namespace te {
class Texture;
}

namespace te::ui {

struct HoverInfo {
  bool hasHover = false;
  Vec2i cell{};
};

struct EditorUIState {
  bool showPalette = true;
  bool showInspector = true;
  bool showLog = true;
  bool showGrid = true;
  bool showFps = false;
  bool vsyncEnabled = true;
  bool vsyncDirty = false;

  std::string currentMapPath = "assets/maps/map.json";
  std::vector<std::string> recentFiles;

  char saveAsBuffer[256]{};
  bool openSaveAsModal = false;
  bool openAboutModal = false;
};

struct EditorUIOutput {
  bool requestSave = false;
  bool requestLoad = false;
  bool requestSaveAs = false;
  bool requestUndo = false;
  bool requestRedo = false;
  bool requestQuit = false;
  bool requestReloadAtlas = false;
  std::string loadPath;
  std::string saveAsPath;
};

void InitEditorUI(EditorUIState& state);
void ShutdownEditorUI(EditorUIState& state);
void AddRecentFile(EditorUIState& state, const std::string& path);
const std::string& GetCurrentMapPath(const EditorUIState& state);

EditorUIOutput DrawEditorUI(EditorUIState& state,
                            EditorState& editor,
                            const OrthoCamera& camera,
                            const HoverInfo& hover,
                            Log& log,
                            const Texture& atlasTexture);

void DrawDockSpace();
void DrawTilePalette(EditorState& state, const Texture& atlasTexture);
void DrawInspector(const EditorState& state, const OrthoCamera& camera, const HoverInfo& hover);
void DrawLog(Log& log);

} // namespace te::ui
