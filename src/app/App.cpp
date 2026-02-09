#include "app/App.h"

#include "render/GL.h"
#include "ui/Panels.h"
#include "util/Log.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <algorithm>
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
  m_atlasTexture.LoadFromFile(m_editor.atlas.path);
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

    m_actions.BeginFrame();
    m_input.BeginFrame();
    m_window.PollEvents();
    m_input.Update(m_window.GetNative());
    m_imgui.NewFrame();

    m_framebuffer = m_window.GetFramebufferSize();

    ImGuiIO& io = ImGui::GetIO();
    const bool imguiActive = ImGui::GetCurrentContext() != nullptr;

    ui::EditorUIOutput uiOutput = ui::DrawEditorUI(m_uiState, m_editor, m_log, m_atlasTexture);
    if (m_uiState.vsyncDirty) {
      m_window.SetVsync(m_uiState.vsyncEnabled);
      m_uiState.vsyncDirty = false;
    }

    const bool blockKeys = imguiActive && io.WantCaptureKeyboard;
    const bool popupOpen = imguiActive && ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
    const bool blockMouse = popupOpen || (imguiActive && io.WantCaptureMouse && ImGui::IsAnyItemHovered());
    const bool allowMouse = m_uiState.sceneHovered && !blockMouse;
    const bool allowKeyboard = !blockKeys;

    const float scaleX = io.DisplayFramebufferScale.x;
    const float scaleY = io.DisplayFramebufferScale.y;
    Vec2i sceneViewport = m_framebuffer;
    int sceneX = 0;
    int sceneY = 0;
    bool hasScene = m_uiState.sceneRect.width > 1.0f && m_uiState.sceneRect.height > 1.0f;
    if (hasScene) {
      const float scenePosX = m_uiState.sceneRect.x * scaleX;
      const float scenePosY = m_uiState.sceneRect.y * scaleY;
      const float sceneWidth = m_uiState.sceneRect.width * scaleX;
      const float sceneHeight = m_uiState.sceneRect.height * scaleY;
      sceneViewport.x = static_cast<int>(sceneWidth);
      sceneViewport.y = static_cast<int>(sceneHeight);
      sceneX = static_cast<int>(scenePosX);
      sceneY = static_cast<int>(static_cast<float>(m_framebuffer.y) - (scenePosY + sceneHeight));
      if (sceneViewport.x <= 0 || sceneViewport.y <= 0) {
        hasScene = false;
        sceneViewport = m_framebuffer;
        sceneX = 0;
        sceneY = 0;
      }
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
        m_atlasTexture.LoadFromFile(m_editor.atlas.path);
        ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);
      } else {
        Log::Error("Failed to load tilemap: " + error);
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
      m_editor.hasUnsavedChanges = true;
      m_uiState.pendingMapWidth = 0;
      m_uiState.pendingMapHeight = 0;
    }

    if (uiOutput.requestReloadAtlas) {
      if (!uiOutput.atlasPath.empty()) {
        m_editor.atlas.path = uiOutput.atlasPath;
      }
      m_atlasTexture.LoadFromFile(m_editor.atlas.path);
      ResolveAtlasGrid(m_editor.atlas, m_atlasTexture);
    }

    const std::string& currentPath = ui::GetCurrentMapPath(m_uiState);
    if (!uiOutput.requestUndo && allowKeyboard && m_actions.Get(Action::Undo).pressed) {
      handleUndo();
    }

    if (!uiOutput.requestRedo && allowKeyboard && m_actions.Get(Action::Redo).pressed) {
      handleRedo();
    }

    if (!uiOutput.requestSave && allowKeyboard && m_actions.Get(Action::Save).pressed) {
      handleSave(currentPath);
    }

    if (!uiOutput.requestLoad && allowKeyboard && m_actions.Get(Action::Load).pressed) {
      handleLoad(currentPath);
    }

    if (!uiOutput.requestQuit && allowKeyboard && m_actions.Get(Action::Quit).pressed) {
      m_window.SetShouldClose(true);
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
      const bool panToolActive = m_editor.currentTool == Tool::Pan;
      const ActionState& panAction = m_actions.Get(Action::PanDrag);
      const ActionState& paintAction = m_actions.Get(Action::Paint);
      if (panAction.down || (panToolActive && paintAction.down)) {
        Vec2 delta = m_input.GetMouseDelta();
        delta.x *= scaleX;
        delta.y *= scaleY;
        camPos.x -= delta.x / m_camera.GetZoom();
        camPos.y += delta.y / m_camera.GetZoom();
      }
    }

    m_camera.SetPosition(camPos);

    Vec2 mouseWorld{};
    if (hasScene) {
      const Vec2 mousePos = m_input.GetMousePos();
      const Vec2 mousePosFb{mousePos.x * scaleX, mousePos.y * scaleY};
      const Vec2 scenePosFb{m_uiState.sceneRect.x * scaleX, m_uiState.sceneRect.y * scaleY};
      const Vec2 localPos{mousePosFb.x - scenePosFb.x, mousePosFb.y - scenePosFb.y};
      mouseWorld = m_camera.ScreenToWorld(localPos, sceneViewport);
    } else {
      const Vec2 mousePos = m_input.GetMousePos();
      const Vec2 mousePosFb{mousePos.x * scaleX, mousePos.y * scaleY};
      mouseWorld = m_camera.ScreenToWorld(mousePosFb, sceneViewport);
    }
    if (!m_uiState.sceneHovered) {
      mouseWorld = {-100000.0f, -100000.0f};
    }

    EditorInput editorInput{};
    editorInput.mouseWorld = mouseWorld;
    const ActionState& paint = m_actions.Get(Action::Paint);
    const ActionState& erase = m_actions.Get(Action::Erase);
    bool leftDown = allowMouse && paint.down;
    bool rightDown = allowMouse && erase.down;
    bool leftPressed = allowMouse && paint.pressed;
    bool rightPressed = allowMouse && erase.pressed;
    bool leftReleased = allowMouse && paint.released;
    bool rightReleased = allowMouse && erase.released;

    if (m_editor.currentTool == Tool::Erase) {
      rightDown = rightDown || leftDown;
      rightPressed = rightPressed || leftPressed;
      rightReleased = rightReleased || leftReleased;
      leftDown = false;
      leftPressed = false;
      leftReleased = false;
    } else if (m_editor.currentTool == Tool::Pan) {
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
    UpdateEditor(m_editor, editorInput);

    glViewport(0, 0, m_framebuffer.x, m_framebuffer.y);
    glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (hasScene) {
      glEnable(GL_SCISSOR_TEST);
      glScissor(sceneX, sceneY, sceneViewport.x, sceneViewport.y);
      glViewport(sceneX, sceneY, sceneViewport.x, sceneViewport.y);
    }

    m_renderer.BeginFrame(m_camera.GetViewProjection(sceneViewport));

    for (int y = 0; y < m_editor.tileMap.GetHeight(); ++y) {
      for (int x = 0; x < m_editor.tileMap.GetWidth(); ++x) {
        const int tileIndex = m_editor.tileMap.GetTile(x, y);
        if (tileIndex == 0) {
          continue;
        }
        const Vec2 pos{static_cast<float>(x * m_editor.tileMap.GetTileSize()),
                       static_cast<float>(y * m_editor.tileMap.GetTileSize())};
        const Vec2 size{static_cast<float>(m_editor.tileMap.GetTileSize()),
                        static_cast<float>(m_editor.tileMap.GetTileSize())};
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
      const float width = static_cast<float>(m_editor.tileMap.GetWidth() * m_editor.tileMap.GetTileSize());
      const float height = static_cast<float>(m_editor.tileMap.GetHeight() * m_editor.tileMap.GetTileSize());
      const Vec4 gridColor{0.15f, 0.15f, 0.18f, 0.7f};

      for (int x = 0; x <= m_editor.tileMap.GetWidth(); ++x) {
        const float xpos = static_cast<float>(x * m_editor.tileMap.GetTileSize());
        m_renderer.DrawLine({xpos, 0.0f}, {xpos, height}, gridColor);
      }

      for (int y = 0; y <= m_editor.tileMap.GetHeight(); ++y) {
        const float ypos = static_cast<float>(y * m_editor.tileMap.GetTileSize());
        m_renderer.DrawLine({0.0f, ypos}, {width, ypos}, gridColor);
      }
    }

    if (m_editor.selection.hasHover) {
      const int x = m_editor.selection.hoverCell.x;
      const int y = m_editor.selection.hoverCell.y;
      const float ts = static_cast<float>(m_editor.tileMap.GetTileSize());
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
    if (hasScene) {
      glDisable(GL_SCISSOR_TEST);
    }

    ui::DrawSceneOverlay(m_uiState, fps, m_camera.GetZoom(), m_editor.selection.hasHover,
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
      handleLoad(loadPath);
    }
    if (uiOutput.requestSaveAs && !uiOutput.saveAsPath.empty()) {
      handleSave(uiOutput.saveAsPath);
    }
    if (uiOutput.requestQuit) {
      m_window.SetShouldClose(true);
    }
    Vec2i windowSize = m_window.GetWindowSize();
    m_uiState.windowWidth = windowSize.x;
    m_uiState.windowHeight = windowSize.y;

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
