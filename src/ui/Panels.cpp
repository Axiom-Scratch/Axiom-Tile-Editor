#include "ui/Panels.h"

#include <imgui.h>

#include <cstdio>
#include <string>

namespace te::ui {

void DrawDockSpace() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

  ImGui::Begin("DockSpace", nullptr, windowFlags);
  ImGui::PopStyleVar(2);

  ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGuiID dockspaceId = ImGui::GetID("DockSpaceRoot");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockFlags);
  ImGui::End();
}

void DrawTilePalette(EditorState& state) {
  ImGui::Begin("Tile Palette");

  ImGui::Text("Select Tile ID");
  for (int i = 1; i <= 9; ++i) {
    ImGui::PushID(i);
    char label[8];
    std::snprintf(label, sizeof(label), "%d", i);
    if (ImGui::Button(label, ImVec2(32.0f, 32.0f))) {
      state.currentTileId = i;
    }
    if ((i % 3) != 0) {
      ImGui::SameLine();
    }
    ImGui::PopID();
  }

  ImGui::Text("Current: %d", state.currentTileId);
  ImGui::End();
}

void DrawInspector(const EditorState& state, const OrthoCamera& camera, const HoverInfo& hover) {
  ImGui::Begin("Inspector");

  if (hover.hasHover) {
    ImGui::Text("Hover: (%d, %d)", hover.cell.x, hover.cell.y);
  } else {
    ImGui::Text("Hover: none");
  }

  ImGui::Text("Tile ID: %d", state.currentTileId);
  ImGui::Text("Zoom: %.2f", camera.GetZoom());

  ImGui::End();
}

void DrawLog(Log& log) {
  ImGui::Begin("Log");

  const auto& lines = log.GetLines();
  for (const std::string& line : lines) {
    ImGui::TextUnformatted(line.c_str());
  }

  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::End();
}

} // namespace te::ui
