#pragma once

#include "app/Config.h"
#include "editor/Tools.h"
#include "platform/Actions.h"
#include "platform/GlfwWindow.h"
#include "platform/Input.h"
#include "render/OrthoCamera.h"
#include "render/Renderer2D.h"
#include "ui/ImGuiLayer.h"
#include "util/Log.h"

namespace te {

class App {
public:
  App() = default;
  ~App() = default;

  bool Init();
  void Run();

private:
  void Shutdown();

  GlfwWindow m_window;
  Actions m_actions;
  Input m_input;
  Renderer2D m_renderer;
  OrthoCamera m_camera;
  EditorState m_editor;
  ImGuiLayer m_imgui;
  Log m_log;
  Vec2i m_framebuffer{};
};

} // namespace te
