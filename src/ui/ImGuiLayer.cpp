#include "ui/ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>

namespace te {

bool ImGuiLayer::Init(GLFWwindow* window) {
  if (m_initialized) {
    return true;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  std::filesystem::create_directories("assets/config");
  static const std::string iniPath = "assets/config/imgui.ini";
  io.IniFilename = iniPath.c_str();

  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    return false;
  }

  m_initialized = true;
  return true;
}

void ImGuiLayer::NewFrame() {
  if (!m_initialized) {
    return;
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::Render() {
  if (!m_initialized) {
    return;
  }
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown() {
  if (!m_initialized) {
    return;
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

} // namespace te
