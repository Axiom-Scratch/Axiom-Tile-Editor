#include "app/App.h"

#include "render/GL.h"
#include "ui/Panels.h"
#include "util/JsonLite.h"
#include "util/Log.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace te {

namespace {

Vec4 TileColor(int id) {
  static const Vec4 palette[9] = {
      {0.90f, 0.20f, 0.20f, 1.0f}, {0.20f, 0.60f, 0.90f, 1.0f}, {0.20f, 0.80f, 0.30f, 1.0f},
      {0.90f, 0.60f, 0.20f, 1.0f}, {0.70f, 0.30f, 0.80f, 1.0f}, {0.30f, 0.80f, 0.80f, 1.0f},
      {0.80f, 0.80f, 0.20f, 1.0f}, {0.90f, 0.40f, 0.60f, 1.0f}, {0.60f, 0.60f, 0.60f, 1.0f},
  };

  if (id <= 0) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  return palette[(id - 1) % 9];
}

void ResolveAtlasGrid(Atlas& atlas, const Texture& texture) {
  if (atlas.tileW < 1) atlas.tileW = 1;
  if (atlas.tileH < 1) atlas.tileH = 1;
  if (texture.GetWidth() > 0 && atlas.cols <= 0) {
    atlas.cols = texture.GetWidth() / atlas.tileW;
  }
  if (texture.GetHeight() > 0 && atlas.rows <= 0) {
    atlas.rows = texture.GetHeight() / atlas.tileH;
  }
  if (atlas.cols < 1) atlas.cols = 1;
  if (atlas.rows < 1) atlas.rows = 1;
}

int StepBrushSize(int current, int direction) {
  const int sizes[] = {1, 2, 4, 8};
  int index = 0;
  for (int i = 0; i < 4; ++i) {
    if (sizes[i] == current) {
      index = i;
      break;
    }
  }
  index = std::clamp(index + direction, 0, 3);
  return sizes[index];
}

std::string SanitizeFileStem(const std::string& name, const std::string& fallback) {
  std::string stem = name;
  if (stem.empty()) {
    stem = fallback;
  }
  for (char& c : stem) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
      c = '_';
    }
  }
  if (stem.empty()) {
    stem = fallback;
  }
  return stem;
}

std::string GetMapStem(const std::string& mapPath) {
  std::filesystem::path path(mapPath);
  std::string stem = path.stem().string();
  return SanitizeFileStem(stem, "map");
}

bool WriteStampFile(const std::string& path, int width, int height, const std::vector<int>& data) {
  std::ostringstream ss;
  ss << "{\n";
  ss << "  \"width\": " << width << ",\n";
  ss << "  \"height\": " << height << ",\n";
  ss << "  \"data\": [";
  for (size_t i = 0; i < data.size(); ++i) {
    ss << data[i];
    if (i + 1 < data.size()) {
      ss << ", ";
    }
  }
  ss << "]\n";
  ss << "}\n";

  return FileIO::WriteTextFile(path, ss.str());
}

bool ReadStampFile(const std::string& path, int& width, int& height, std::vector<int>& data,
                   std::string* errorOut) {
  std::string text;
  if (!FileIO::ReadTextFile(path, text)) {
    if (errorOut) {
      *errorOut = "Failed to read stamp file.";
    }
    return false;
  }
  if (!JsonLite::ParseIntAfterKey(text, "width", width) ||
      !JsonLite::ParseIntAfterKey(text, "height", height)) {
    if (errorOut) {
      *errorOut = "Stamp missing width/height.";
    }
    return false;
  }
  if (!JsonLite::ParseDataArray(text, "data", data)) {
    if (errorOut) {
      *errorOut = "Stamp missing data array.";
    }
    return false;
  }
  if (width <= 0 || height <= 0 || static_cast<int>(data.size()) != width * height) {
    if (errorOut) {
      *errorOut = "Stamp data size mismatch.";
    }
    return false;
  }
  return true;
}

bool WriteCsvFile(const std::string& path, const std::vector<int>& data, int width, int height) {
  if (width <= 0 || height <= 0) {
    return false;
  }
  if (static_cast<int>(data.size()) < width * height) {
    return false;
  }
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int index = y * width + x;
      file << data[static_cast<size_t>(index)];
      if (x + 1 < width) {
        file << ',';
      }
    }
    file << '\n';
  }
  return true;
}

bool ReadCsvFile(const std::string& path, int width, int height, std::vector<int>& outData,
                 std::string* errorOut) {
  std::ifstream file(path);
  if (!file) {
    if (errorOut) {
      *errorOut = "Failed to open CSV file.";
    }
    return false;
  }
  outData.assign(static_cast<size_t>(width * height), 0);
  std::string line;
  int y = 0;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    if (y >= height) {
      if (errorOut) {
        *errorOut = "CSV has more rows than map height.";
      }
      return false;
    }
    std::stringstream ss(line);
    std::string cell;
    int x = 0;
    while (std::getline(ss, cell, ',')) {
      if (x >= width) {
        if (errorOut) {
          *errorOut = "CSV row has more columns than map width.";
        }
        return false;
      }
      std::stringstream cellStream(cell);
      int value = 0;
      if (!(cellStream >> value)) {
        if (errorOut) {
          *errorOut = "Invalid CSV cell value.";
        }
        return false;
      }
      outData[static_cast<size_t>(y * width + x)] = value;
      ++x;
    }
    if (x != width) {
      if (errorOut) {
        *errorOut = "CSV row width mismatch.";
      }
      return false;
    }
    ++y;
  }
  if (y != height) {
    if (errorOut) {
      *errorOut = "CSV row count mismatch.";
    }
    return false;
  }
  return true;
}

bool ComputeSelectionBounds(const EditorState& editor, Vec2i& outMin, Vec2i& outMax) {
  if (!editor.selection.HasSelection()) {
    return false;
  }
  const int width = editor.tileMap.GetWidth();
  int minX = width;
  int minY = editor.tileMap.GetHeight();
  int maxX = 0;
  int maxY = 0;
  for (int index : editor.selection.indices) {
    if (index < 0) {
      continue;
    }
    const int x = index % width;
    const int y = index / width;
    minX = std::min(minX, x);
    minY = std::min(minY, y);
    maxX = std::max(maxX, x);
    maxY = std::max(maxY, y);
  }
  if (minX > maxX || minY > maxY) {
    return false;
  }
  outMin = {minX, minY};
  outMax = {maxX, maxY};
  return true;
}

bool ComputeAtlasUV(const Atlas& atlas, int tileIndex, Vec2& uv0, Vec2& uv1) {
  if (tileIndex <= 0) {
    return false;
  }
  const int cols = std::max(1, atlas.cols);
  const int rows = std::max(1, atlas.rows);
  const int idx = tileIndex - 1;
  const int col = idx % cols;
  const int row = idx / cols;
  if (row >= rows) {
    return false;
  }
  const float u0 = static_cast<float>(col) / static_cast<float>(cols);
  const float v0 = static_cast<float>(row) / static_cast<float>(rows);
  const float u1 = static_cast<float>(col + 1) / static_cast<float>(cols);
  const float v1 = static_cast<float>(row + 1) / static_cast<float>(rows);
  uv0 = {u0, v0};
  uv1 = {u1, v1};
  return true;
}

int GetTileSelectAction(const Actions& actions) {
  if (actions.Get(Action::Tile1).pressed) return 1;
  if (actions.Get(Action::Tile2).pressed) return 2;
  if (actions.Get(Action::Tile3).pressed) return 3;
  if (actions.Get(Action::Tile4).pressed) return 4;
  if (actions.Get(Action::Tile5).pressed) return 5;
  if (actions.Get(Action::Tile6).pressed) return 6;
  if (actions.Get(Action::Tile7).pressed) return 7;
  if (actions.Get(Action::Tile8).pressed) return 8;
  if (actions.Get(Action::Tile9).pressed) return 9;
  return 0;
}

} // namespace

bool App::Init() {
  ui::LoadEditorConfig(m_uiState);
  const int windowWidth = m_uiState.windowWidth > 0 ? m_uiState.windowWidth : AppConfig::WindowWidth;
  const int windowHeight = m_uiState.windowHeight > 0 ? m_uiState.windowHeight : AppConfig::WindowHeight;
  if (!m_window.Create(windowWidth, windowHeight, AppConfig::WindowTitle)) {
    return false;
  }
  m_windowTitle = AppConfig::WindowTitle;
  m_window.SetTitle(m_windowTitle.c_str());
  m_lastDirty = false;

  m_window.SetVsync(m_uiState.vsyncEnabled);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    Log::Error("Failed to initialize GLAD.");
    return false;
  }

  const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  Log::Info(std::string("OpenGL: ") + (glVersion ? glVersion : "Unknown"));

#if defined(TE_DEBUG) && defined(TE_ENABLE_GL_DEBUG)
  gl::EnableDebugOutput();
#endif

  glEnable(GL_FRAMEBUFFER_SRGB);

  if (!m_renderer.Init()) {
    Log::Error("Failed to initialize renderer.");
    return false;
  }

  InitEditor(m_editor, AppConfig::MapWidth, AppConfig::MapHeight, AppConfig::TileSize);
  if (!m_uiState.lastAtlas.path.empty()) {
    m_editor.atlas = m_uiState.lastAtlas;
  }
  if (m_editor.atlas.path.empty()) {
    m_editor.atlas.path = "assets/textures/atlas.png";
  }
  if (m_editor.atlas.tileW <= 0) {
    m_editor.atlas.tileW = m_editor.tileMap.GetTileSize();
  }
  if (m_editor.atlas.tileH <= 0) {
    m_editor.atlas.tileH = m_editor.tileMap.GetTileSize();
  }
  m_atlasLoaded = m_atlasTexture.LoadFromFile(m_editor.atlas.path);
  m_loadedAtlasPath = m_editor.atlas.path;
  if (m_atlasLoaded) {
    Log::Info("Loaded atlas " + m_loadedAtlasPath + " (" + std::to_string(m_atlasTexture.GetWidth()) + "x" +
              std::to_string(m_atlasTexture.GetHeight()) + ")");
  }
  ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);

  m_input.SetActions(&m_actions);
  m_input.Attach(m_window.GetNative());
  if (!m_imgui.Init(m_window.GetNative())) {
    Log::Error("Failed to initialize ImGui.");
    return false;
  }

  const float mapWorldWidth = static_cast<float>(AppConfig::MapWidth * AppConfig::TileSize);
  const float mapWorldHeight = static_cast<float>(AppConfig::MapHeight * AppConfig::TileSize);
  m_camera.SetPosition({mapWorldWidth * 0.5f, mapWorldHeight * 0.5f});
  m_camera.SetZoom(1.0f);

  return true;
}

void App::Run() {
  if (!Init()) {
    Shutdown();
    return;
  }

  double lastTime = glfwGetTime();
  while (!m_window.ShouldClose()) {
    const double now = glfwGetTime();
    const float dt = static_cast<float>(now - lastTime);
    const float fps = dt > 0.0f ? (1.0f / dt) : 0.0f;
    lastTime = now;
    if (m_uiState.saveMessageTimer > 0.0f) {
      m_uiState.saveMessageTimer = std::max(0.0f, m_uiState.saveMessageTimer - dt);
    }

    m_actions.BeginFrame();
    m_input.BeginFrame();
    m_window.PollEvents();
    m_input.Update(m_window.GetNative());
    m_imgui.NewFrame();

    m_framebuffer = m_window.GetFramebufferSize();

    ImGuiIO& io = ImGui::GetIO();
    const bool imguiActive = ImGui::GetCurrentContext() != nullptr;

    ui::EditorUIOutput uiOutput =
        ui::DrawEditorUI(m_uiState, m_editor, m_log, m_atlasTexture, m_sceneFramebuffer, m_camera.GetZoom(), fps);
    if (m_uiState.vsyncDirty) {
      m_window.SetVsync(m_uiState.vsyncEnabled);
      m_uiState.vsyncDirty = false;
    }

    const bool blockKeys = imguiActive && io.WantCaptureKeyboard;
    const bool sceneHovered = uiOutput.sceneHovered;
    const bool imguiCapturingMouse = io.WantCaptureMouse && !sceneHovered;
    const bool allowMouse = sceneHovered && !imguiCapturingMouse;
    const bool allowKeyboard = !blockKeys;

    const bool ctrlDown = m_input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || m_input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
    const bool shiftDown = m_input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || m_input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
    const bool altDown = m_input.IsKeyDown(GLFW_KEY_LEFT_ALT) || m_input.IsKeyDown(GLFW_KEY_RIGHT_ALT);
    if (allowKeyboard) {
      if (m_input.WasKeyPressed(GLFW_KEY_Q)) m_editor.currentTool = Tool::Paint;
      if (m_input.WasKeyPressed(GLFW_KEY_W)) m_editor.currentTool = Tool::Rect;
      if (m_input.WasKeyPressed(GLFW_KEY_E)) m_editor.currentTool = Tool::Fill;
      if (m_input.WasKeyPressed(GLFW_KEY_R)) m_editor.currentTool = Tool::Erase;
      if (m_input.WasKeyPressed(GLFW_KEY_I)) {
        if (m_editor.currentTool == Tool::Pick) {
          m_editor.currentTool = m_editor.previousTool;
        } else {
          m_editor.previousTool = m_editor.currentTool;
          m_editor.currentTool = Tool::Pick;
        }
      }
      if (m_input.WasKeyPressed(GLFW_KEY_LEFT_BRACKET)) {
        m_editor.brushSize = StepBrushSize(m_editor.brushSize, -1);
      }
      if (m_input.WasKeyPressed(GLFW_KEY_RIGHT_BRACKET)) {
        m_editor.brushSize = StepBrushSize(m_editor.brushSize, 1);
      }
    }

    if (allowKeyboard && ctrlDown && shiftDown && m_input.WasKeyPressed(GLFW_KEY_S)) {
      m_uiState.showSaveAs = true;
    }

    const bool panHold = allowKeyboard && m_input.IsKeyDown(GLFW_KEY_SPACE);
    const Tool activeTool = panHold ? Tool::Pan : m_editor.currentTool;

    Vec2i sceneViewport{m_sceneFramebuffer.GetWidth(), m_sceneFramebuffer.GetHeight()};
    const Vec2 sceneRectMin = uiOutput.sceneRectMin;
    const Vec2 sceneRectMax = uiOutput.sceneRectMax;
    const float sceneWidth = sceneRectMax.x - sceneRectMin.x;
    const float sceneHeight = sceneRectMax.y - sceneRectMin.y;
    bool hasScene = sceneWidth > 1.0f && sceneHeight > 1.0f && sceneViewport.x > 0 && sceneViewport.y > 0;
    float sceneScaleX = 1.0f;
    float sceneScaleY = 1.0f;
    if (hasScene) {
      sceneScaleX = static_cast<float>(sceneViewport.x) / sceneWidth;
      sceneScaleY = static_cast<float>(sceneViewport.y) / sceneHeight;
    }

    auto frameSelection = [&]() {
      const int mapWidth = m_editor.tileMap.GetWidth();
      const int mapHeight = m_editor.tileMap.GetHeight();
      const int tileSize = m_editor.tileMap.GetTileSize();
      Vec2i boundsMin{};
      Vec2i boundsMax{};
      const bool hasSelection = ComputeSelectionBounds(m_editor, boundsMin, boundsMax);
      float targetWidth = static_cast<float>(mapWidth * tileSize);
      float targetHeight = static_cast<float>(mapHeight * tileSize);
      float centerX = targetWidth * 0.5f;
      float centerY = targetHeight * 0.5f;
      if (hasSelection && tileSize > 0) {
        const int selWidth = boundsMax.x - boundsMin.x + 1;
        const int selHeight = boundsMax.y - boundsMin.y + 1;
        targetWidth = static_cast<float>(selWidth * tileSize);
        targetHeight = static_cast<float>(selHeight * tileSize);
        centerX = static_cast<float>(boundsMin.x + boundsMax.x + 1) * 0.5f * static_cast<float>(tileSize);
        centerY = static_cast<float>(boundsMin.y + boundsMax.y + 1) * 0.5f * static_cast<float>(tileSize);
      }

      if (targetWidth <= 0.0f) {
        targetWidth = 1.0f;
      }
      if (targetHeight <= 0.0f) {
        targetHeight = 1.0f;
      }
      if (hasScene && sceneViewport.x > 0 && sceneViewport.y > 0) {
        const float zoomX = static_cast<float>(sceneViewport.x) / targetWidth;
        const float zoomY = static_cast<float>(sceneViewport.y) / targetHeight;
        float zoom = std::min(zoomX, zoomY) * 0.9f;
        zoom = std::clamp(zoom, 0.1f, 8.0f);
        m_camera.SetZoom(zoom);
      }
      m_camera.SetPosition({centerX, centerY});
    };
    if (allowKeyboard && m_input.WasKeyPressed(GLFW_KEY_F)) {
      frameSelection();
    }
    if (uiOutput.requestFrame) {
      frameSelection();
    }

    auto handleUndo = [&]() {
      EndStroke(m_editor);
      Undo(m_editor);
    };

    auto handleRedo = [&]() {
      EndStroke(m_editor);
      Redo(m_editor);
    };

    auto handleSave = [&](const std::string& path) {
      EndStroke(m_editor);
      if (path.empty()) {
        m_uiState.showSaveAs = true;
        return;
      }
      if (SaveTileMap(m_editor, path)) {
        Log::Info("Saved tilemap to " + path);
        ui::AddRecentFile(m_uiState, path);
        m_editor.hasUnsavedChanges = false;
        m_uiState.saveMessageTimer = 1.0f;
      } else {
        Log::Error("Failed to save tilemap.");
      }
    };

    auto handleLoad = [&](const std::string& path) {
      EndStroke(m_editor);
      std::string error;
      if (LoadTileMap(m_editor, path, &error)) {
        Log::Info("Loaded tilemap from " + path);
        ui::AddRecentFile(m_uiState, path);
        if (!m_atlasLoaded || m_loadedAtlasPath != m_editor.atlas.path) {
          m_atlasLoaded = m_atlasTexture.LoadFromFile(m_editor.atlas.path);
          m_loadedAtlasPath = m_editor.atlas.path;
          if (m_atlasLoaded) {
            Log::Info("Loaded atlas " + m_loadedAtlasPath + " (" + std::to_string(m_atlasTexture.GetWidth()) + "x" +
                      std::to_string(m_atlasTexture.GetHeight()) + ")");
          }
          ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);
        }
      } else {
        Log::Error("Failed to load tilemap: " + error);
      }
    };

    auto handleNewMap = [&]() {
      EndStroke(m_editor);
      InitEditor(m_editor, AppConfig::MapWidth, AppConfig::MapHeight, AppConfig::TileSize);
      if (!m_uiState.lastAtlas.path.empty()) {
        m_editor.atlas = m_uiState.lastAtlas;
      }
      if (m_editor.atlas.path.empty()) {
        m_editor.atlas.path = "assets/textures/atlas.png";
      }
      if (m_editor.atlas.tileW <= 0) {
        m_editor.atlas.tileW = m_editor.tileMap.GetTileSize();
      }
      if (m_editor.atlas.tileH <= 0) {
        m_editor.atlas.tileH = m_editor.tileMap.GetTileSize();
      }
      m_atlasLoaded = m_atlasTexture.LoadFromFile(m_editor.atlas.path);
      m_loadedAtlasPath = m_editor.atlas.path;
      if (m_atlasLoaded) {
        Log::Info("Loaded atlas " + m_loadedAtlasPath + " (" + std::to_string(m_atlasTexture.GetWidth()) + "x" +
                  std::to_string(m_atlasTexture.GetHeight()) + ")");
      }
      ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);
      m_editor.hasUnsavedChanges = false;
      m_uiState.currentMapPath = "assets/maps/untitled.json";
    };

    auto requestLoad = [&](const std::string& path) {
      if (m_editor.hasUnsavedChanges) {
        m_uiState.pendingAction = ui::PendingAction::LoadPath;
        m_uiState.pendingLoadPath = path;
        m_uiState.showConfirmOpen = true;
      } else {
        handleLoad(path);
      }
    };

    auto requestQuit = [&]() {
      if (m_editor.hasUnsavedChanges) {
        m_uiState.pendingAction = ui::PendingAction::Quit;
        m_uiState.pendingLoadPath.clear();
        m_uiState.showConfirmQuit = true;
      } else {
        m_window.SetShouldClose(true);
      }
    };

    if (uiOutput.requestFocus) {
      const float mapWorldWidth = static_cast<float>(m_editor.tileMap.GetWidth() * m_editor.tileMap.GetTileSize());
      const float mapWorldHeight = static_cast<float>(m_editor.tileMap.GetHeight() * m_editor.tileMap.GetTileSize());
      m_camera.SetPosition({mapWorldWidth * 0.5f, mapWorldHeight * 0.5f});
      m_camera.SetZoom(1.0f);
    }

    if (uiOutput.requestSetZoom) {
      float zoom = std::clamp(uiOutput.zoomValue, 0.2f, 4.0f);
      m_camera.SetZoom(zoom);
    }

    if (uiOutput.requestResizeMap) {
      EndStroke(m_editor);
      SetMapSize(m_editor, uiOutput.resizeWidth, uiOutput.resizeHeight);
      m_uiState.pendingMapWidth = 0;
      m_uiState.pendingMapHeight = 0;
    }

    if (uiOutput.requestReloadAtlas) {
      if (!uiOutput.atlasPath.empty()) {
        m_editor.atlas.path = uiOutput.atlasPath;
      }
      m_atlasLoaded = m_atlasTexture.LoadFromFile(m_editor.atlas.path);
      m_loadedAtlasPath = m_editor.atlas.path;
      if (m_atlasLoaded) {
        Log::Info("Loaded atlas " + m_loadedAtlasPath + " (" + std::to_string(m_atlasTexture.GetWidth()) + "x" +
                  std::to_string(m_atlasTexture.GetHeight()) + ")");
      }
      ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);
    }

    const std::string& currentPath = ui::GetCurrentMapPath(m_uiState);
    if (m_editor.hasUnsavedChanges && m_uiState.autosaveEnabled) {
      m_uiState.autosaveTimer += dt;
      if (m_uiState.autosaveTimer >= m_uiState.autosaveInterval) {
        const std::string autosavePath =
            m_uiState.autosavePath.empty() ? "assets/autosave/autosave.json" : m_uiState.autosavePath;
        if (SaveTileMap(m_editor, autosavePath)) {
          Log::Info("Autosaved tilemap to " + autosavePath);
        } else {
          Log::Warn("Failed to autosave tilemap.");
        }
        m_uiState.autosaveTimer = 0.0f;
      }
    } else {
      m_uiState.autosaveTimer = 0.0f;
    }
    if (!uiOutput.requestUndo && allowKeyboard && m_actions.Get(Action::Undo).pressed) {
      handleUndo();
    }

    if (!uiOutput.requestRedo && allowKeyboard && m_actions.Get(Action::Redo).pressed) {
      handleRedo();
    }

    if (!uiOutput.requestSave && allowKeyboard && !shiftDown && m_actions.Get(Action::Save).pressed) {
      handleSave(currentPath);
    }

    if (!uiOutput.requestLoad && allowKeyboard && m_actions.Get(Action::Load).pressed) {
      requestLoad(currentPath);
    }

    if (!uiOutput.requestQuit && allowKeyboard && m_actions.Get(Action::Quit).pressed) {
      requestQuit();
    }

    const float moveSpeed = 600.0f / m_camera.GetZoom();
    Vec2 camPos = m_camera.GetPosition();
    if (allowKeyboard) {
      if (m_actions.Get(Action::MoveUp).down) camPos.y += moveSpeed * dt;
      if (m_actions.Get(Action::MoveDown).down) camPos.y -= moveSpeed * dt;
      if (m_actions.Get(Action::MoveLeft).down) camPos.x -= moveSpeed * dt;
      if (m_actions.Get(Action::MoveRight).down) camPos.x += moveSpeed * dt;
    }

    m_camera.SetPosition(camPos);
    const bool sceneWidgetActive = io.WantCaptureMouse && uiOutput.sceneActive;
    const bool allowZoom = sceneHovered && hasScene && !sceneWidgetActive;
    float scroll = m_actions.Get(Action::ZoomIn).value - m_actions.Get(Action::ZoomOut).value;
    if (m_uiState.invertZoom) {
      scroll = -scroll;
    }
    constexpr float kMinZoom = 0.1f;
    constexpr float kMaxZoom = 8.0f;
    if (allowZoom && scroll != 0.0f) {
      ImVec2 mousePos = ImGui::GetMousePos();
      const Vec2 localPos{mousePos.x - sceneRectMin.x, mousePos.y - sceneRectMin.y};
      Vec2 uv{0.0f, 0.0f};
      if (sceneWidth > 0.0f && sceneHeight > 0.0f) {
        uv.x = localPos.x / sceneWidth;
        uv.y = localPos.y / sceneHeight;
      }
      uv.x = std::clamp(uv.x, 0.0f, 1.0f);
      uv.y = std::clamp(uv.y, 0.0f, 1.0f);
      const Vec2 localFb{uv.x * static_cast<float>(sceneViewport.x),
                         uv.y * static_cast<float>(sceneViewport.y)};

      const Vec2 worldBefore = m_camera.ScreenToWorld(localFb, sceneViewport);
      const float zoomStep = 1.1f;
      float zoom = m_camera.GetZoom();
      zoom = std::clamp(zoom * std::pow(zoomStep, scroll), kMinZoom, kMaxZoom);
      m_camera.SetZoom(zoom);
      const Vec2 worldAfter = m_camera.ScreenToWorld(localFb, sceneViewport);
      camPos.x += worldBefore.x - worldAfter.x;
      camPos.y += worldBefore.y - worldAfter.y;
      m_camera.SetPosition(camPos);
    }

    if (allowMouse) {
      const bool panToolActive = activeTool == Tool::Pan;
      const ActionState& panAction = m_actions.Get(Action::PanDrag);
      const ActionState& paintAction = m_actions.Get(Action::Paint);
      const bool altPan = altDown && paintAction.down;
      if (panAction.down || altPan || (panToolActive && paintAction.down)) {
        Vec2 delta = m_input.GetMouseDelta();
        delta.x *= sceneScaleX * m_uiState.panSpeed;
        delta.y *= sceneScaleY * m_uiState.panSpeed;
        camPos.x -= delta.x / m_camera.GetZoom();
        camPos.y += delta.y / m_camera.GetZoom();
      }
    }

    m_camera.SetPosition(camPos);

    const bool sceneInputActive = allowMouse && hasScene && sceneHovered;
    Vec2 mouseWorld{-1.0f, -1.0f};
    if (sceneInputActive) {
      const ImVec2 mousePos = ImGui::GetMousePos();
      const Vec2 localPos{mousePos.x - sceneRectMin.x, mousePos.y - sceneRectMin.y};
      Vec2 uv{0.0f, 0.0f};
      if (sceneWidth > 0.0f && sceneHeight > 0.0f) {
        uv.x = localPos.x / sceneWidth;
        uv.y = localPos.y / sceneHeight;
      }
      uv.x = std::clamp(uv.x, 0.0f, 1.0f);
      uv.y = std::clamp(uv.y, 0.0f, 1.0f);
      const Vec2 localFb{uv.x * static_cast<float>(sceneViewport.x),
                         uv.y * static_cast<float>(sceneViewport.y)};
      mouseWorld = m_camera.ScreenToWorld(localFb, sceneViewport);
    }

    EditorInput editorInput{};
    editorInput.mouseWorld = mouseWorld;
    bool leftDown = sceneInputActive && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool leftPressed = sceneInputActive && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool leftReleased = sceneInputActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    bool rightDown = sceneInputActive && ImGui::IsMouseDown(ImGuiMouseButton_Right);
    bool rightPressed = sceneInputActive && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool rightReleased = sceneInputActive && ImGui::IsMouseReleased(ImGuiMouseButton_Right);

    const bool altPanActive = altDown && leftDown;
    if (activeTool == Tool::Pan || altPanActive) {
      leftDown = false;
      leftPressed = false;
      leftReleased = false;
      rightDown = false;
      rightPressed = false;
      rightReleased = false;
    }

    editorInput.leftDown = leftDown;
    editorInput.rightDown = rightDown;
    editorInput.leftPressed = leftPressed;
    editorInput.rightPressed = rightPressed;
    editorInput.leftReleased = leftReleased;
    editorInput.rightReleased = rightReleased;
    editorInput.tileSelect = allowKeyboard ? GetTileSelectAction(m_actions) : 0;
    editorInput.shift = shiftDown;
    editorInput.ctrl = ctrlDown;
    UpdateEditor(m_editor, editorInput);

    glViewport(0, 0, m_framebuffer.x, m_framebuffer.y);
    glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const int mapWidth = m_editor.tileMap.GetWidth();
    const int mapHeight = m_editor.tileMap.GetHeight();
    const int tileSize = m_editor.tileMap.GetTileSize();
    const float mapWorldWidth = static_cast<float>(mapWidth * tileSize);
    const float mapWorldHeight = static_cast<float>(mapHeight * tileSize);
    float viewLeft = 0.0f;
    float viewRight = 0.0f;
    float viewBottom = 0.0f;
    float viewTop = 0.0f;
    bool viewValid = false;

    if (hasScene) {
      m_sceneFramebuffer.Bind();
      glViewport(0, 0, sceneViewport.x, sceneViewport.y);
      glClearColor(m_editor.sceneBgColor.r, m_editor.sceneBgColor.g, m_editor.sceneBgColor.b,
                   m_editor.sceneBgColor.a);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      m_renderer.BeginFrame(m_camera.GetViewProjection(sceneViewport));
      int minX = 0;
      int maxX = mapWidth - 1;
      int minY = 0;
      int maxY = mapHeight - 1;
      if (sceneViewport.x > 0 && sceneViewport.y > 0 && tileSize > 0) {
        const float halfW = static_cast<float>(sceneViewport.x) * 0.5f / m_camera.GetZoom();
        const float halfH = static_cast<float>(sceneViewport.y) * 0.5f / m_camera.GetZoom();
        const Vec2 camPosNow = m_camera.GetPosition();
        viewLeft = camPosNow.x - halfW;
        viewRight = camPosNow.x + halfW;
        viewBottom = camPosNow.y - halfH;
        viewTop = camPosNow.y + halfH;
        minX = std::max(0, static_cast<int>(std::floor(viewLeft / static_cast<float>(tileSize))) - 1);
        maxX = std::min(mapWidth - 1, static_cast<int>(std::ceil(viewRight / static_cast<float>(tileSize))) + 1);
        minY = std::max(0, static_cast<int>(std::floor(viewBottom / static_cast<float>(tileSize))) - 1);
        maxY = std::min(mapHeight - 1, static_cast<int>(std::ceil(viewTop / static_cast<float>(tileSize))) + 1);
        viewValid = true;
      }

      for (size_t layerIndex = 0; layerIndex < m_editor.layers.size(); ++layerIndex) {
        const Layer& layer = m_editor.layers[layerIndex];
        if (!layer.visible) {
          continue;
        }
        if (layer.tiles.size() < static_cast<size_t>(mapWidth * mapHeight)) {
          continue;
        }
        const float alpha = std::clamp(layer.opacity, 0.0f, 1.0f);
        for (int y = minY; y <= maxY; ++y) {
          for (int x = minX; x <= maxX; ++x) {
            const int index = y * mapWidth + x;
            const int tileIndex = layer.tiles[static_cast<size_t>(index)];
            if (tileIndex == 0) {
              continue;
            }
            const Vec2 pos{static_cast<float>(x * tileSize), static_cast<float>(y * tileSize)};
            const Vec2 size{static_cast<float>(tileSize), static_cast<float>(tileSize)};
            if (!m_atlasTexture.IsFallback()) {
              Vec2 uv0{};
              Vec2 uv1{};
              if (ComputeAtlasUV(m_editor.atlas, tileIndex, uv0, uv1)) {
                m_renderer.DrawQuad(pos, size, {1.0f, 1.0f, 1.0f, alpha}, uv0, uv1, &m_atlasTexture);
              } else {
                Vec4 color = TileColor(tileIndex);
                color.a *= alpha;
                m_renderer.DrawQuad(pos, size, color);
              }
            } else {
              Vec4 color = TileColor(tileIndex);
              color.a *= alpha;
              m_renderer.DrawQuad(pos, size, color);
            }
          }
        }
      }

      if (m_uiState.showGrid) {
        const float width = mapWorldWidth;
        const float height = mapWorldHeight;
        const float cellSize = (m_uiState.gridCellSize > 0.0f) ? m_uiState.gridCellSize
                                                              : static_cast<float>(tileSize);
        const int majorStep = std::max(1, m_uiState.gridMajorStep);
        Vec4 gridColor = m_uiState.gridColor;
        gridColor.a = m_uiState.gridAlpha;
        Vec4 majorColor = gridColor;
        majorColor.a = std::min(1.0f, gridColor.a * 1.5f);

        const int cellsX = (cellSize > 0.0f) ? static_cast<int>(std::ceil(width / cellSize)) : 0;
        const int cellsY = (cellSize > 0.0f) ? static_cast<int>(std::ceil(height / cellSize)) : 0;

        for (int x = 0; x <= cellsX; ++x) {
          const float xpos = static_cast<float>(x) * cellSize;
          const Vec4 color = (x % majorStep == 0) ? majorColor : gridColor;
          m_renderer.DrawLine({xpos, 0.0f}, {xpos, height}, color);
        }

        for (int y = 0; y <= cellsY; ++y) {
          const float ypos = static_cast<float>(y) * cellSize;
          const Vec4 color = (y % majorStep == 0) ? majorColor : gridColor;
          m_renderer.DrawLine({0.0f, ypos}, {width, ypos}, color);
        }
      }

      if (viewValid) {
        const Vec4 axisColor{0.35f, 0.35f, 0.40f, 0.6f};
        m_renderer.DrawLine({0.0f, viewBottom}, {0.0f, viewTop}, axisColor);
        m_renderer.DrawLine({viewLeft, 0.0f}, {viewRight, 0.0f}, axisColor);
      }

      if (m_editor.selection.HasSelection()) {
        const Vec4 fillColor{0.20f, 0.55f, 1.0f, 0.25f};
        const Vec4 borderColor{0.35f, 0.70f, 1.0f, 0.9f};
        for (int index : m_editor.selection.indices) {
          const int x = index % mapWidth;
          const int y = index / mapWidth;
          if (x < minX || x > maxX || y < minY || y > maxY) {
            continue;
          }
          const float x0 = static_cast<float>(x * tileSize);
          const float y0 = static_cast<float>(y * tileSize);
          const float x1 = x0 + static_cast<float>(tileSize);
          const float y1 = y0 + static_cast<float>(tileSize);
          m_renderer.DrawQuad({x0, y0}, {static_cast<float>(tileSize), static_cast<float>(tileSize)}, fillColor);
          m_renderer.DrawLine({x0, y0}, {x1, y0}, borderColor);
          m_renderer.DrawLine({x1, y0}, {x1, y1}, borderColor);
          m_renderer.DrawLine({x1, y1}, {x0, y1}, borderColor);
          m_renderer.DrawLine({x0, y1}, {x0, y0}, borderColor);
        }
      }

      if (m_editor.selection.isSelecting) {
        const Vec2i a = m_editor.selection.selectStart;
        const Vec2i b = m_editor.selection.selectEnd;
        const int minSelX = std::min(a.x, b.x);
        const int maxSelX = std::max(a.x, b.x);
        const int minSelY = std::min(a.y, b.y);
        const int maxSelY = std::max(a.y, b.y);
        const float x0 = static_cast<float>(minSelX * tileSize);
        const float y0 = static_cast<float>(minSelY * tileSize);
        const float x1 = static_cast<float>((maxSelX + 1) * tileSize);
        const float y1 = static_cast<float>((maxSelY + 1) * tileSize);
        const Vec4 fillColor{0.20f, 0.55f, 1.0f, 0.15f};
        const Vec4 borderColor{0.35f, 0.70f, 1.0f, 0.9f};
        m_renderer.DrawQuad({x0, y0}, {x1 - x0, y1 - y0}, fillColor);
        m_renderer.DrawLine({x0, y0}, {x1, y0}, borderColor);
        m_renderer.DrawLine({x1, y0}, {x1, y1}, borderColor);
        m_renderer.DrawLine({x1, y1}, {x0, y1}, borderColor);
        m_renderer.DrawLine({x0, y1}, {x0, y0}, borderColor);
      }

      if (m_editor.rectActive) {
        const Vec2i a = m_editor.rectStart;
        const Vec2i b = m_editor.rectEnd;
        const int minRectX = std::min(a.x, b.x);
        const int maxRectX = std::max(a.x, b.x);
        const int minRectY = std::min(a.y, b.y);
        const int maxRectY = std::max(a.y, b.y);
        const float x0 = static_cast<float>(minRectX * tileSize);
        const float y0 = static_cast<float>(minRectY * tileSize);
        const float x1 = static_cast<float>((maxRectX + 1) * tileSize);
        const float y1 = static_cast<float>((maxRectY + 1) * tileSize);
        const Vec4 fillColor = m_editor.rectErase ? Vec4{0.90f, 0.35f, 0.35f, 0.18f}
                                                  : Vec4{0.35f, 0.90f, 0.45f, 0.18f};
        const Vec4 borderColor = m_editor.rectErase ? Vec4{0.95f, 0.45f, 0.45f, 0.9f}
                                                    : Vec4{0.45f, 0.95f, 0.55f, 0.9f};
        m_renderer.DrawQuad({x0, y0}, {x1 - x0, y1 - y0}, fillColor);
        m_renderer.DrawLine({x0, y0}, {x1, y0}, borderColor);
        m_renderer.DrawLine({x1, y0}, {x1, y1}, borderColor);
        m_renderer.DrawLine({x1, y1}, {x0, y1}, borderColor);
        m_renderer.DrawLine({x0, y1}, {x0, y0}, borderColor);
      }

      if (m_editor.lineActive) {
        std::vector<Vec2i> lineCells;
        BuildLineCells(m_editor.lineStart, m_editor.lineEnd, lineCells);
        const Vec4 fillColor{0.95f, 0.90f, 0.35f, 0.18f};
        const Vec4 borderColor{0.95f, 0.90f, 0.35f, 0.9f};
        for (const Vec2i& cell : lineCells) {
          if (!m_editor.tileMap.IsInBounds(cell.x, cell.y)) {
            continue;
          }
          if (cell.x < minX || cell.x > maxX || cell.y < minY || cell.y > maxY) {
            continue;
          }
          const float x0 = static_cast<float>(cell.x * tileSize);
          const float y0 = static_cast<float>(cell.y * tileSize);
          const float x1 = x0 + static_cast<float>(tileSize);
          const float y1 = y0 + static_cast<float>(tileSize);
          m_renderer.DrawQuad({x0, y0}, {static_cast<float>(tileSize), static_cast<float>(tileSize)}, fillColor);
          m_renderer.DrawLine({x0, y0}, {x1, y0}, borderColor);
          m_renderer.DrawLine({x1, y0}, {x1, y1}, borderColor);
          m_renderer.DrawLine({x1, y1}, {x0, y1}, borderColor);
          m_renderer.DrawLine({x0, y1}, {x0, y0}, borderColor);
        }
      }

      if (m_editor.selection.hasHover) {
        const int size = std::max(1, m_editor.brushSize);
        const int half = size / 2;
        const int startX = m_editor.selection.hoverCell.x - half;
        const int startY = m_editor.selection.hoverCell.y - half;
        const int brushMinX = std::max(0, startX);
        const int brushMinY = std::max(0, startY);
        const int brushMaxX = std::min(mapWidth, startX + size);
        const int brushMaxY = std::min(mapHeight, startY + size);
        const float ts = static_cast<float>(tileSize);
        const float x0 = static_cast<float>(brushMinX) * ts;
        const float y0 = static_cast<float>(brushMinY) * ts;
        const float x1 = static_cast<float>(brushMaxX) * ts;
        const float y1 = static_cast<float>(brushMaxY) * ts;
        const Vec4 hoverColor{1.0f, 1.0f, 1.0f, 0.4f};
        m_renderer.DrawLine({x0, y0}, {x1, y0}, hoverColor);
        m_renderer.DrawLine({x1, y0}, {x1, y1}, hoverColor);
        m_renderer.DrawLine({x1, y1}, {x0, y1}, hoverColor);
        m_renderer.DrawLine({x0, y1}, {x0, y0}, hoverColor);
      }

      m_renderer.EndFrame();
      m_sceneFramebuffer.Unbind();
      glViewport(0, 0, m_framebuffer.x, m_framebuffer.y);
    }

    ui::DrawSceneOverlay(m_uiState, m_editor, m_atlasTexture, m_camera.GetPosition(), m_camera.GetZoom(),
                         mapWorldWidth, mapWorldHeight, viewLeft, viewRight, viewBottom, viewTop);
    if (uiOutput.requestLoadStamp && !uiOutput.stampPath.empty()) {
      int stampWidth = 0;
      int stampHeight = 0;
      std::vector<int> stampData;
      std::string error;
      if (ReadStampFile(uiOutput.stampPath, stampWidth, stampHeight, stampData, &error)) {
        m_editor.stampWidth = stampWidth;
        m_editor.stampHeight = stampHeight;
        m_editor.stampTiles = std::move(stampData);
        m_editor.previousTool = m_editor.currentTool;
        m_editor.currentTool = Tool::Stamp;
        Log::Info("Loaded stamp: " + uiOutput.stampPath);
      } else {
        Log::Error("Failed to load stamp: " + error);
      }
    }
    if (uiOutput.requestCreateStamp) {
      EndStroke(m_editor);
      Vec2i boundsMin{};
      Vec2i boundsMax{};
      if (!ComputeSelectionBounds(m_editor, boundsMin, boundsMax)) {
        Log::Warn("Select tiles before creating a stamp.");
      } else {
        const int width = boundsMax.x - boundsMin.x + 1;
        const int height = boundsMax.y - boundsMin.y + 1;
        std::vector<int> stampData(static_cast<size_t>(width * height), 0);
        const int layerIndex = (m_editor.activeLayer >= 0 &&
                                m_editor.activeLayer < static_cast<int>(m_editor.layers.size()))
                                   ? m_editor.activeLayer
                                   : 0;
        if (layerIndex >= 0 && layerIndex < static_cast<int>(m_editor.layers.size())) {
          const Layer& layer = m_editor.layers[static_cast<size_t>(layerIndex)];
          for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
              const int cellX = boundsMin.x + x;
              const int cellY = boundsMin.y + y;
              if (!m_editor.tileMap.IsInBounds(cellX, cellY)) {
                continue;
              }
              const int index = m_editor.tileMap.Index(cellX, cellY);
              if (index >= 0 && index < static_cast<int>(layer.tiles.size())) {
                stampData[static_cast<size_t>(y * width + x)] = layer.tiles[static_cast<size_t>(index)];
              }
            }
          }
        }

        std::filesystem::create_directories("assets/stamps");
        const std::string baseName = SanitizeFileStem(uiOutput.stampName, "stamp");
        std::filesystem::path target = std::filesystem::path("assets/stamps") / (baseName + ".json");
        for (int i = 1; std::filesystem::exists(target) && i < 1000; ++i) {
          target = std::filesystem::path("assets/stamps") /
                   (baseName + "_" + std::to_string(i) + ".json");
        }
        if (WriteStampFile(target.generic_string(), width, height, stampData)) {
          Log::Info("Saved stamp: " + target.generic_string());
        } else {
          Log::Error("Failed to save stamp.");
        }
      }
    }
    if (uiOutput.requestExportCsv) {
      EndStroke(m_editor);
      const int width = m_editor.tileMap.GetWidth();
      const int height = m_editor.tileMap.GetHeight();
      const int layerIndex = (m_editor.activeLayer >= 0 &&
                              m_editor.activeLayer < static_cast<int>(m_editor.layers.size()))
                                 ? m_editor.activeLayer
                                 : 0;
      if (layerIndex < 0 || layerIndex >= static_cast<int>(m_editor.layers.size())) {
        Log::Warn("No active layer to export.");
      } else {
        const Layer& layer = m_editor.layers[static_cast<size_t>(layerIndex)];
        if (static_cast<int>(layer.tiles.size()) < width * height) {
          Log::Error("Layer data size mismatch.");
        } else {
          std::filesystem::create_directories("assets/exports");
          const std::string stem = GetMapStem(ui::GetCurrentMapPath(m_uiState));
          std::filesystem::path csvPath = std::filesystem::path("assets/exports") / (stem + ".csv");
          if (WriteCsvFile(csvPath.generic_string(), layer.tiles, width, height)) {
            Log::Info("Exported CSV: " + csvPath.generic_string());
          } else {
            Log::Error("Failed to export CSV.");
          }
        }
      }
    }
    if (uiOutput.requestImportCsv) {
      EndStroke(m_editor);
      const int width = m_editor.tileMap.GetWidth();
      const int height = m_editor.tileMap.GetHeight();
      const int layerIndex = (m_editor.activeLayer >= 0 &&
                              m_editor.activeLayer < static_cast<int>(m_editor.layers.size()))
                                 ? m_editor.activeLayer
                                 : 0;
      if (layerIndex < 0 || layerIndex >= static_cast<int>(m_editor.layers.size())) {
        Log::Warn("No active layer to import into.");
      } else if (m_editor.layers[static_cast<size_t>(layerIndex)].locked) {
        Log::Warn("Active layer is locked.");
      } else {
        const std::string stem = GetMapStem(ui::GetCurrentMapPath(m_uiState));
        std::filesystem::path csvPath = std::filesystem::path("assets/exports") / (stem + ".csv");
        std::vector<int> csvData;
        std::string error;
        if (!ReadCsvFile(csvPath.generic_string(), width, height, csvData, &error)) {
          Log::Error("Failed to import CSV: " + error);
        } else {
          Layer& layer = m_editor.layers[static_cast<size_t>(layerIndex)];
          if (static_cast<int>(layer.tiles.size()) < width * height) {
            layer.tiles.assign(static_cast<size_t>(width * height), 0);
          }
          PaintCommand command;
          command.layerIndex = layerIndex;
          command.mapWidth = width;
          for (int i = 0; i < width * height; ++i) {
            const int before = layer.tiles[static_cast<size_t>(i)];
            const int after = csvData[static_cast<size_t>(i)];
            if (before == after) {
              continue;
            }
            layer.tiles[static_cast<size_t>(i)] = after;
            AddOrUpdateChange(command, i, before, after);
          }
          if (!command.changes.empty()) {
            m_editor.history.Push(std::move(command));
            m_editor.hasUnsavedChanges = true;
          }
          Log::Info("Imported CSV: " + csvPath.generic_string());
        }
      }
    }
    if (uiOutput.requestUndo) {
      handleUndo();
    }
    if (uiOutput.requestRedo) {
      handleRedo();
    }
    if (uiOutput.requestSave) {
      handleSave(ui::GetCurrentMapPath(m_uiState));
    }
    if (uiOutput.requestLoad) {
      const std::string& loadPath = uiOutput.loadPath.empty() ? ui::GetCurrentMapPath(m_uiState) : uiOutput.loadPath;
      requestLoad(loadPath);
    }
    if (uiOutput.requestSaveAs && !uiOutput.saveAsPath.empty()) {
      handleSave(uiOutput.saveAsPath);
    }
    if (uiOutput.requestNewMap) {
      handleNewMap();
    }
    if (uiOutput.confirmSave) {
      handleSave(ui::GetCurrentMapPath(m_uiState));
      if (m_uiState.pendingAction == ui::PendingAction::Quit) {
        m_window.SetShouldClose(true);
      } else if (m_uiState.pendingAction == ui::PendingAction::LoadPath) {
        handleLoad(m_uiState.pendingLoadPath);
      } else if (m_uiState.pendingAction == ui::PendingAction::NewMap) {
        handleNewMap();
      } else if (m_uiState.pendingAction == ui::PendingAction::OpenPicker) {
        m_uiState.showOpenModal = true;
      }
      m_uiState.pendingAction = ui::PendingAction::None;
      m_uiState.pendingLoadPath.clear();
    }
    if (uiOutput.confirmDiscard) {
      if (m_uiState.pendingAction == ui::PendingAction::Quit) {
        m_window.SetShouldClose(true);
      } else if (m_uiState.pendingAction == ui::PendingAction::LoadPath) {
        handleLoad(m_uiState.pendingLoadPath);
      } else if (m_uiState.pendingAction == ui::PendingAction::NewMap) {
        handleNewMap();
      } else if (m_uiState.pendingAction == ui::PendingAction::OpenPicker) {
        m_editor.hasUnsavedChanges = false;
        m_uiState.showOpenModal = true;
      }
      m_uiState.pendingAction = ui::PendingAction::None;
      m_uiState.pendingLoadPath.clear();
    }
    if (uiOutput.requestQuit) {
      requestQuit();
    }
    Vec2i windowSize = m_window.GetWindowSize();
    m_uiState.windowWidth = windowSize.x;
    m_uiState.windowHeight = windowSize.y;
    if (m_editor.hasUnsavedChanges != m_lastDirty) {
      std::string title = m_windowTitle;
      if (m_editor.hasUnsavedChanges) {
        title += " *";
      }
      m_window.SetTitle(title.c_str());
      m_lastDirty = m_editor.hasUnsavedChanges;
    }

    m_imgui.Render();

    m_window.SwapBuffers();
  }

  Shutdown();
}

void App::Shutdown() {
  Vec2i windowSize = m_window.GetWindowSize();
  m_uiState.windowWidth = windowSize.x;
  m_uiState.windowHeight = windowSize.y;
  m_uiState.lastAtlas = m_editor.atlas;
  ui::SaveEditorConfig(m_uiState);
  m_imgui.Shutdown();
  m_renderer.Shutdown();
  m_window.Destroy();
}

} // namespace te
