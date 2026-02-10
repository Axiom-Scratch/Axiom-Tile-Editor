#include "app/App.h"

#include "render/GL.h"
#include "ui/Panels.h"
#include "util/Log.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <string>

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
        ui::DrawEditorUI(m_uiState, m_editor, m_log, m_atlasTexture, m_sceneFramebuffer, m_camera.GetZoom());
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
    if (allowKeyboard) {
      if (m_input.WasKeyPressed(GLFW_KEY_Q)) m_editor.currentTool = Tool::Paint;
      if (m_input.WasKeyPressed(GLFW_KEY_W)) m_editor.currentTool = Tool::Rect;
      if (m_input.WasKeyPressed(GLFW_KEY_E)) m_editor.currentTool = Tool::Fill;
      if (m_input.WasKeyPressed(GLFW_KEY_R)) m_editor.currentTool = Tool::Erase;
    }

    if (allowKeyboard && ctrlDown && shiftDown && m_input.WasKeyPressed(GLFW_KEY_S)) {
      m_uiState.openSaveAsModal = true;
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

    auto requestLoad = [&](const std::string& path) {
      if (m_editor.hasUnsavedChanges) {
        m_uiState.pendingLoadPath = path;
        m_uiState.pendingQuit = false;
        m_uiState.openUnsavedModal = true;
      } else {
        handleLoad(path);
      }
    };

    auto requestQuit = [&]() {
      if (m_editor.hasUnsavedChanges) {
        m_uiState.pendingQuit = true;
        m_uiState.pendingLoadPath.clear();
        m_uiState.openUnsavedModal = true;
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

    if (uiOutput.requestResizeMap) {
      EndStroke(m_editor);
      m_editor.tileMap.Resize(uiOutput.resizeWidth, uiOutput.resizeHeight, m_editor.tileMap.GetTileSize());
      m_editor.history.Clear();
      m_editor.selection.Resize(uiOutput.resizeWidth, uiOutput.resizeHeight);
      m_editor.hasUnsavedChanges = true;
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
        const std::string autosavePath = currentPath.empty() ? "assets/maps/autosave.json" : currentPath + ".autosave";
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

    if (allowMouse) {
      const float scroll = m_actions.Get(Action::ZoomIn).value - m_actions.Get(Action::ZoomOut).value;
      if (scroll != 0.0f) {
        float zoom = m_camera.GetZoom();
        zoom *= 1.0f + scroll * 0.1f;
        zoom = std::clamp(zoom, 0.2f, 4.0f);
        m_camera.SetZoom(zoom);
      }
    }

    if (allowMouse) {
      const bool panToolActive = activeTool == Tool::Pan;
      const ActionState& panAction = m_actions.Get(Action::PanDrag);
      const ActionState& paintAction = m_actions.Get(Action::Paint);
      if (panAction.down || (panToolActive && paintAction.down)) {
        Vec2 delta = m_input.GetMouseDelta();
        delta.x *= sceneScaleX;
        delta.y *= sceneScaleY;
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

    if (activeTool == Tool::Pan) {
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

    if (hasScene) {
      m_sceneFramebuffer.Bind();
      glViewport(0, 0, sceneViewport.x, sceneViewport.y);
      glClearColor(0.18f, 0.18f, 0.20f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      m_renderer.BeginFrame(m_camera.GetViewProjection(sceneViewport));

      const int mapWidth = m_editor.tileMap.GetWidth();
      const int mapHeight = m_editor.tileMap.GetHeight();
      const int tileSize = m_editor.tileMap.GetTileSize();
      int minX = 0;
      int maxX = mapWidth - 1;
      int minY = 0;
      int maxY = mapHeight - 1;
      if (sceneViewport.x > 0 && sceneViewport.y > 0 && tileSize > 0) {
        const float halfW = static_cast<float>(sceneViewport.x) * 0.5f / m_camera.GetZoom();
        const float halfH = static_cast<float>(sceneViewport.y) * 0.5f / m_camera.GetZoom();
        const Vec2 camPosNow = m_camera.GetPosition();
        const float left = camPosNow.x - halfW;
        const float right = camPosNow.x + halfW;
        const float bottom = camPosNow.y - halfH;
        const float top = camPosNow.y + halfH;
        minX = std::max(0, static_cast<int>(std::floor(left / static_cast<float>(tileSize))) - 1);
        maxX = std::min(mapWidth - 1, static_cast<int>(std::ceil(right / static_cast<float>(tileSize))) + 1);
        minY = std::max(0, static_cast<int>(std::floor(bottom / static_cast<float>(tileSize))) - 1);
        maxY = std::min(mapHeight - 1, static_cast<int>(std::ceil(top / static_cast<float>(tileSize))) + 1);
      }

      for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
          const int tileIndex = m_editor.tileMap.GetTile(x, y);
          if (tileIndex == 0) {
            continue;
          }
          const Vec2 pos{static_cast<float>(x * tileSize), static_cast<float>(y * tileSize)};
          const Vec2 size{static_cast<float>(tileSize), static_cast<float>(tileSize)};
          if (!m_atlasTexture.IsFallback()) {
            Vec2 uv0{};
            Vec2 uv1{};
            if (ComputeAtlasUV(m_editor.atlas, tileIndex, uv0, uv1)) {
              m_renderer.DrawQuad(pos, size, {1.0f, 1.0f, 1.0f, 1.0f}, uv0, uv1, &m_atlasTexture);
            } else {
              const Vec4 color = TileColor(tileIndex);
              m_renderer.DrawQuad(pos, size, color);
            }
          } else {
            const Vec4 color = TileColor(tileIndex);
            m_renderer.DrawQuad(pos, size, color);
          }
        }
      }

      if (m_uiState.showGrid) {
        const float width = static_cast<float>(mapWidth * tileSize);
        const float height = static_cast<float>(mapHeight * tileSize);
        const Vec4 gridColor{0.15f, 0.15f, 0.18f, 0.7f};

        for (int x = minX; x <= maxX + 1; ++x) {
          const float xpos = static_cast<float>(x * tileSize);
          m_renderer.DrawLine({xpos, 0.0f}, {xpos, height}, gridColor);
        }

        for (int y = minY; y <= maxY + 1; ++y) {
          const float ypos = static_cast<float>(y * tileSize);
          m_renderer.DrawLine({0.0f, ypos}, {width, ypos}, gridColor);
        }
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

      if (m_editor.selection.hasHover) {
        const int x = m_editor.selection.hoverCell.x;
        const int y = m_editor.selection.hoverCell.y;
        const float ts = static_cast<float>(tileSize);
        const float x0 = static_cast<float>(x) * ts;
        const float y0 = static_cast<float>(y) * ts;
        const float x1 = x0 + ts;
        const float y1 = y0 + ts;
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

    const Vec2 rawMousePos = m_input.GetMousePos();
    const Vec2 rawMouseDelta = m_input.GetMouseDelta();
    const Vec2 rawScrollDelta = m_input.GetScrollDelta();
    const bool lmbDown = m_input.IsMouseDown(GLFW_MOUSE_BUTTON_LEFT);
    const bool rmbDown = m_input.IsMouseDown(GLFW_MOUSE_BUTTON_RIGHT);
    const bool mmbDown = m_input.IsMouseDown(GLFW_MOUSE_BUTTON_MIDDLE);
    ui::DrawSceneOverlay(m_uiState, fps, m_camera.GetZoom(), m_editor.currentTool, rawMousePos, rawMouseDelta,
                         rawScrollDelta, lmbDown, rmbDown, mmbDown, m_editor.selection.hasHover,
                         m_editor.selection.hoverCell, m_editor.currentTileIndex);
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
    if (uiOutput.confirmSave) {
      handleSave(ui::GetCurrentMapPath(m_uiState));
      if (m_uiState.pendingQuit) {
        m_window.SetShouldClose(true);
      } else if (!m_uiState.pendingLoadPath.empty()) {
        handleLoad(m_uiState.pendingLoadPath);
      }
      m_uiState.pendingLoadPath.clear();
      m_uiState.pendingQuit = false;
    }
    if (uiOutput.confirmDiscard) {
      if (m_uiState.pendingQuit) {
        m_window.SetShouldClose(true);
      } else if (!m_uiState.pendingLoadPath.empty()) {
        handleLoad(m_uiState.pendingLoadPath);
      }
      m_uiState.pendingLoadPath.clear();
      m_uiState.pendingQuit = false;
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
