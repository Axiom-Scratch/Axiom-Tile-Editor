#pragma once

#include "app/Config.h"
#include "editor/Atlas.h"
#include "editor/Tools.h"
#include "ui/Theme.h"
#include "util/Log.h"

#include <string>
#include <vector>

namespace te {
class Texture;
class Framebuffer;
}

namespace te::ui {

struct SceneViewRect {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
};

struct EditorUIState {
  std::string currentMapPath;
  std::vector<std::string> recentFiles;
  Atlas lastAtlas{};
  int windowWidth = 0;
  int windowHeight = 0;

  bool showHierarchy = true;
  bool showInspector = true;
  bool showSettings = true;
  bool showProject = true;
  bool showConsole = true;
  bool showTilePalette = true;
  bool showGrid = true;

  bool showFps = true;
  bool vsyncEnabled = true;
  bool vsyncDirty = false;
  bool snapEnabled = false;

  bool filterInfo = true;
  bool filterWarn = true;
  bool filterError = true;
  char consoleFilter[128]{};

  char saveAsBuffer[256]{};
  char atlasPathBuffer[256]{};
  char layerNameBuffer[64]{};
  int pendingMapWidth = 0;
  int pendingMapHeight = 0;

  bool openSaveAsModal = false;
  bool openAboutModal = false;
  bool openResizeModal = false;
  bool openUnsavedModal = false;

  std::string pendingLoadPath;
  bool pendingQuit = false;

  SceneViewRect sceneRect{};
  bool sceneHovered = false;

  bool dockInitialized = false;
  int lastLayerSelection = -2;

  ThemeSettings theme{};
  bool themeDirty = true;

  float saveMessageTimer = 0.0f;
  bool autosaveEnabled = false;
  float autosaveInterval = 60.0f;
  float autosaveTimer = 0.0f;
};

struct EditorUIOutput {
  bool requestSave = false;
  bool requestLoad = false;
  bool requestSaveAs = false;
  bool requestUndo = false;
  bool requestRedo = false;
  bool requestQuit = false;
  bool requestReloadAtlas = false;
  bool requestFocus = false;
  bool requestResizeMap = false;
  bool confirmSave = false;
  bool confirmDiscard = false;

  std::string loadPath;
  std::string saveAsPath;
  std::string atlasPath;
  int resizeWidth = 0;
  int resizeHeight = 0;

  Vec2 sceneRectMin{};
  Vec2 sceneRectMax{};
  bool sceneHovered = false;
  bool sceneActive = false;
};

void LoadEditorConfig(EditorUIState& state);
void SaveEditorConfig(const EditorUIState& state);
void AddRecentFile(EditorUIState& state, const std::string& path);
const std::string& GetCurrentMapPath(const EditorUIState& state);

EditorUIOutput DrawEditorUI(EditorUIState& state,
                            EditorState& editor,
                            Log& log,
                            const Texture& atlasTexture,
                            Framebuffer& sceneFramebuffer,
                            float cameraZoom);

void DrawSceneOverlay(const EditorUIState& state,
                      float fps,
                      float zoom,
                      Tool currentTool,
                      Vec2 mousePos,
                      Vec2 mouseDelta,
                      Vec2 scrollDelta,
                      bool lmbDown,
                      bool rmbDown,
                      bool mmbDown,
                      bool hasHover,
                      Vec2i hoverCell,
                      int tileIndex);

} // namespace te::ui
