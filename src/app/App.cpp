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
  if (!m_window.Create(AppConfig::WindowWidth, AppConfig::WindowHeight, AppConfig::WindowTitle)) {
    return false;
  }

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

  InitEditor(m_editor, AppConfig::MapWidth, AppConfig::MapHeight, AppConfig::TileSize);

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
    lastTime = now;

    m_actions.BeginFrame();
    m_input.BeginFrame();
    m_window.PollEvents();
    m_input.Update(m_window.GetNative());
    m_imgui.NewFrame();

    m_framebuffer = m_window.GetFramebufferSize();
    glViewport(0, 0, m_framebuffer.x, m_framebuffer.y);

    ImGuiIO& io = ImGui::GetIO();
    const bool imguiActive = ImGui::GetCurrentContext() != nullptr;
    const bool blockMouse = imguiActive && io.WantCaptureMouse;
    const bool blockKeys = imguiActive && io.WantCaptureKeyboard;
    const bool allowMouse = !blockMouse;
    const bool allowKeyboard = !blockKeys;

    if (allowKeyboard && m_actions.Get(Action::Undo).pressed) {
      EndStroke(m_editor);
      Undo(m_editor);
    }

    if (allowKeyboard && m_actions.Get(Action::Redo).pressed) {
      EndStroke(m_editor);
      Redo(m_editor);
    }

    if (allowKeyboard && m_actions.Get(Action::Save).pressed) {
      EndStroke(m_editor);
      if (SaveTileMap(m_editor, "assets/maps/map.json")) {
        Log::Info("Saved tilemap to assets/maps/map.json");
      } else {
        Log::Error("Failed to save tilemap.");
      }
    }

    if (allowKeyboard && m_actions.Get(Action::Load).pressed) {
      EndStroke(m_editor);
      std::string error;
      if (LoadTileMap(m_editor, "assets/maps/map.json", &error)) {
        Log::Info("Loaded tilemap from assets/maps/map.json");
      } else {
        Log::Error("Failed to load tilemap: " + error);
      }
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

    if (allowMouse && m_actions.Get(Action::PanDrag).down) {
      Vec2 delta = m_input.GetMouseDelta();
      camPos.x -= delta.x / m_camera.GetZoom();
      camPos.y += delta.y / m_camera.GetZoom();
    }

    m_camera.SetPosition(camPos);

    const Vec2 mouseWorld = m_camera.ScreenToWorld(m_input.GetMousePos(), m_framebuffer);

    EditorInput editorInput{};
    editorInput.mouseWorld = mouseWorld;
    const ActionState& paint = m_actions.Get(Action::Paint);
    const ActionState& erase = m_actions.Get(Action::Erase);
    editorInput.leftDown = allowMouse && paint.down;
    editorInput.rightDown = allowMouse && erase.down;
    editorInput.leftPressed = allowMouse && paint.pressed;
    editorInput.rightPressed = allowMouse && erase.pressed;
    editorInput.leftReleased = allowMouse && paint.released;
    editorInput.rightReleased = allowMouse && erase.released;
    editorInput.tileSelect = allowKeyboard ? GetTileSelectAction(m_actions) : 0;
    UpdateEditor(m_editor, editorInput);

    glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_renderer.BeginFrame(m_camera.GetViewProjection(m_framebuffer));

    for (int y = 0; y < m_editor.tileMap.GetHeight(); ++y) {
      for (int x = 0; x < m_editor.tileMap.GetWidth(); ++x) {
        const int id = m_editor.tileMap.GetTile(x, y);
        if (id == 0) {
          continue;
        }
        const Vec4 color = TileColor(id);
        const Vec2 pos{static_cast<float>(x * m_editor.tileMap.GetTileSize()),
                       static_cast<float>(y * m_editor.tileMap.GetTileSize())};
        const Vec2 size{static_cast<float>(m_editor.tileMap.GetTileSize()),
                        static_cast<float>(m_editor.tileMap.GetTileSize())};
        m_renderer.DrawQuad(pos, size, color);
      }
    }

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

    ui::HoverInfo hoverInfo{m_editor.selection.hasHover, m_editor.selection.hoverCell};
    ui::DrawDockSpace();
    ui::DrawTilePalette(m_editor);
    ui::DrawInspector(m_editor, m_camera, hoverInfo);
    ui::DrawLog(m_log);
    m_imgui.Render();

    m_window.SwapBuffers();
  }

  Shutdown();
}

void App::Shutdown() {
  m_imgui.Shutdown();
  m_renderer.Shutdown();
  m_window.Destroy();
}

} // namespace te
