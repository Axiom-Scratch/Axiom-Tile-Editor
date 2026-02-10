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

enum class PendingAction {
  None,
  NewMap,
  OpenPicker,
  LoadPath,
  Quit
};

struct EditorUIState {
  std::string currentMapPath;
  std::vector<std::string> recentFiles;
  Atlas lastAtlas{};
  int windowWidth = 0;
  int windowHeight = 0;

  bool showHierarchy = true;
  bool showScene = true;
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

  bool openResizeModal = false;
  bool openDeleteModal = false;
  bool openLayerDeleteModal = false;
  bool openStampModal = false;
  bool showPreferences = false;
  bool showAbout = false;
  bool showConfirmQuit = false;
  bool showConfirmOpen = false;
  bool showRecoverAutosave = false;
  bool showSaveAs = false;
  bool showOpenModal = false;
  bool showOverwriteModal = false;

  PendingAction pendingAction = PendingAction::None;
  std::string pendingLoadPath;
  std::string pendingDeletePath;
  std::string pendingOverwritePath;
  int pendingLayerDeleteIndex = -1;
  char stampNameBuffer[128]{};

  SceneViewRect sceneRect{};
  bool sceneHovered = false;

  bool dockInitialized = false;
  bool requestResetLayout = false;
  int lastLayerSelection = -2;

  ThemeSettings theme{};
  bool themeDirty = true;

  float saveMessageTimer = 0.0f;
  bool autosaveEnabled = false;
  float autosaveInterval = 60.0f;
  float autosaveTimer = 0.0f;
  std::string autosavePath;
  char autosavePathBuffer[256]{};

  float gridCellSize = 0.0f;
  int gridMajorStep = 8;
  Vec4 gridColor{0.15f, 0.15f, 0.18f, 1.0f};
  float gridAlpha = 0.7f;

  bool invertZoom = false;
  float panSpeed = 1.0f;

  char projectFilter[128]{};
  int projectFilterMode = 0;

  bool consoleCollapse = false;
  int consoleSelectedIndex = -1;
  std::string consoleSelectedMessage;
};

struct EditorUIOutput {
  bool requestSave = false;
  bool requestLoad = false;
  bool requestNewMap = false;
  bool requestSaveAs = false;
  bool requestUndo = false;
  bool requestRedo = false;
  bool requestQuit = false;
  bool requestReloadAtlas = false;
  bool requestFocus = false;
  bool requestFrame = false;
  bool requestResizeMap = false;
  bool confirmSave = false;
  bool confirmDiscard = false;
  bool requestSetZoom = false;
  bool requestExportCsv = false;
  bool requestImportCsv = false;
  bool requestLoadStamp = false;
  bool requestCreateStamp = false;

  std::string loadPath;
  std::string saveAsPath;
  std::string atlasPath;
  std::string stampPath;
  std::string stampName;
  int resizeWidth = 0;
  int resizeHeight = 0;
  float zoomValue = 1.0f;

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
                            float cameraZoom,
                            float fps);

void DrawSceneOverlay(const EditorUIState& state,
                      const EditorState& editor,
                      const Texture& atlasTexture,
                      const Vec2& cameraPos,
                      float zoom,
                      float mapWorldWidth,
                      float mapWorldHeight,
                      float viewLeft,
                      float viewRight,
                      float viewBottom,
                      float viewTop);

} // namespace te::ui
