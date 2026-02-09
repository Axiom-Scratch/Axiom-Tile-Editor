#include "ui/Panels.h"

#include "render/Texture.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace te::ui {

namespace {

constexpr const char* kEditorConfigPath = "assets/config/editor.json";

ImVec4 TileFallbackColor(int index) {
  static const ImVec4 palette[9] = {
      {0.90f, 0.20f, 0.20f, 1.0f}, {0.20f, 0.60f, 0.90f, 1.0f}, {0.20f, 0.80f, 0.30f, 1.0f},
      {0.90f, 0.60f, 0.20f, 1.0f}, {0.70f, 0.30f, 0.80f, 1.0f}, {0.30f, 0.80f, 0.80f, 1.0f},
      {0.80f, 0.80f, 0.20f, 1.0f}, {0.90f, 0.40f, 0.60f, 1.0f}, {0.60f, 0.60f, 0.60f, 1.0f},
  };
  if (index <= 0) {
    return {0.2f, 0.2f, 0.2f, 1.0f};
  }
  return palette[(index - 1) % 9];
}

bool ComputeAtlasUV(const Atlas& atlas, int tileIndex, ImVec2& uv0, ImVec2& uv1) {
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

void DrawTileButtons(EditorState& state, const Texture& atlasTexture) {
  ImGui::Text("Select Tile");
  const int cols = std::max(1, state.atlas.cols);
  const int rows = std::max(1, state.atlas.rows);
  const int total = cols * rows;
  const float buttonSize = 36.0f;

  ImTextureID textureId = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(atlasTexture.GetId()));
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      const int tileIndex = row * cols + col + 1;
      if (tileIndex > total) {
        break;
      }
      ImGui::PushID(tileIndex);
      bool clicked = false;
      if (atlasTexture.IsFallback()) {
        ImVec4 color = TileFallbackColor(tileIndex);
        clicked = ImGui::ColorButton("##tile", color, ImGuiColorEditFlags_NoTooltip,
                                      ImVec2(buttonSize, buttonSize));
      } else {
        ImVec2 uv0{};
        ImVec2 uv1{};
        if (ComputeAtlasUV(state.atlas, tileIndex, uv0, uv1)) {
          clicked = ImGui::ImageButton(textureId, ImVec2(buttonSize, buttonSize), uv0, uv1);
        } else {
          clicked = ImGui::Button("?", ImVec2(buttonSize, buttonSize));
        }
      }
      if (clicked) {
        state.currentTileIndex = tileIndex;
      }
      if (col + 1 < cols) {
        ImGui::SameLine();
      }
      ImGui::PopID();
    }
  }

  ImGui::Text("Current: %d", state.currentTileIndex);
}

std::string EscapeJson(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (char c : value) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

std::string UnescapeJson(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  bool escape = false;
  for (char c : value) {
    if (escape) {
      out.push_back(c);
      escape = false;
      continue;
    }
    if (c == '\\') {
      escape = true;
      continue;
    }
    out.push_back(c);
  }
  return out;
}

std::string ExtractJsonString(const std::string& json, const char* key) {
  const std::string token = std::string("\"") + key + "\"";
  size_t pos = json.find(token);
  if (pos == std::string::npos) {
    return {};
  }
  pos = json.find(':', pos);
  if (pos == std::string::npos) {
    return {};
  }
  pos = json.find('"', pos);
  if (pos == std::string::npos) {
    return {};
  }
  ++pos;
  std::string value;
  bool escape = false;
  for (; pos < json.size(); ++pos) {
    char c = json[pos];
    if (!escape && c == '"') {
      break;
    }
    if (!escape && c == '\\') {
      escape = true;
      continue;
    }
    escape = false;
    value.push_back(c);
  }
  return UnescapeJson(value);
}

std::vector<std::string> ExtractJsonArray(const std::string& json, const char* key) {
  std::vector<std::string> result;
  const std::string token = std::string("\"") + key + "\"";
  size_t pos = json.find(token);
  if (pos == std::string::npos) {
    return result;
  }
  pos = json.find('[', pos);
  if (pos == std::string::npos) {
    return result;
  }
  size_t end = json.find(']', pos);
  if (end == std::string::npos) {
    return result;
  }
  bool inString = false;
  bool escape = false;
  std::string current;
  for (size_t i = pos + 1; i < end; ++i) {
    char c = json[i];
    if (!inString) {
      if (c == '"') {
        inString = true;
        current.clear();
      }
      continue;
    }
    if (escape) {
      current.push_back(c);
      escape = false;
      continue;
    }
    if (c == '\\') {
      escape = true;
      continue;
    }
    if (c == '"') {
      inString = false;
      result.push_back(UnescapeJson(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  return result;
}

void SaveEditorConfig(const EditorUIState& state) {
  std::filesystem::path path(kEditorConfigPath);
  std::filesystem::create_directories(path.parent_path());

  std::ofstream file(kEditorConfigPath, std::ios::trunc);
  if (!file) {
    return;
  }

  file << "{\n";
  file << "  \"current\": \"" << EscapeJson(state.currentMapPath) << "\",\n";
  file << "  \"recent\": [\n";
  for (size_t i = 0; i < state.recentFiles.size(); ++i) {
    file << "    \"" << EscapeJson(state.recentFiles[i]) << "\"";
    if (i + 1 < state.recentFiles.size()) {
      file << ",";
    }
    file << "\n";
  }
  file << "  ]\n";
  file << "}\n";
}

void LoadEditorConfig(EditorUIState& state) {
  std::ifstream file(kEditorConfigPath);
  if (!file) {
    return;
  }
  std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  std::string current = ExtractJsonString(json, "current");
  if (!current.empty()) {
    state.currentMapPath = current;
  }
  state.recentFiles = ExtractJsonArray(json, "recent");
  if (state.recentFiles.size() > 10) {
    state.recentFiles.resize(10);
  }
}

void OpenSaveAsModal(EditorUIState& state) {
  std::snprintf(state.saveAsBuffer, sizeof(state.saveAsBuffer), "%s", state.currentMapPath.c_str());
  state.openSaveAsModal = true;
}

} // namespace

void InitEditorUI(EditorUIState& state) {
  state = {};
  state.currentMapPath = "assets/maps/map.json";
  std::snprintf(state.saveAsBuffer, sizeof(state.saveAsBuffer), "%s", state.currentMapPath.c_str());
  LoadEditorConfig(state);
  if (state.currentMapPath.empty() && !state.recentFiles.empty()) {
    state.currentMapPath = state.recentFiles.front();
  }
  if (state.currentMapPath.empty()) {
    state.currentMapPath = "assets/maps/map.json";
  }
  std::snprintf(state.saveAsBuffer, sizeof(state.saveAsBuffer), "%s", state.currentMapPath.c_str());
}

void ShutdownEditorUI(EditorUIState& state) {
  SaveEditorConfig(state);
}

void AddRecentFile(EditorUIState& state, const std::string& path) {
  if (path.empty()) {
    return;
  }

  state.currentMapPath = path;
  std::snprintf(state.saveAsBuffer, sizeof(state.saveAsBuffer), "%s", state.currentMapPath.c_str());
  auto it = std::find(state.recentFiles.begin(), state.recentFiles.end(), path);
  if (it != state.recentFiles.end()) {
    state.recentFiles.erase(it);
  }
  state.recentFiles.insert(state.recentFiles.begin(), path);
  if (state.recentFiles.size() > 10) {
    state.recentFiles.resize(10);
  }
  SaveEditorConfig(state);
}

const std::string& GetCurrentMapPath(const EditorUIState& state) {
  return state.currentMapPath;
}

EditorUIOutput DrawEditorUI(EditorUIState& state,
                            EditorState& editor,
                            const OrthoCamera& camera,
                            const HoverInfo& hover,
                            Log& log,
                            const Texture& atlasTexture) {
  EditorUIOutput out{};

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
                                 ImGuiWindowFlags_MenuBar;

  ImGui::Begin("DockSpace", nullptr, windowFlags);
  ImGui::PopStyleVar(2);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save", "Ctrl+S")) {
        out.requestSave = true;
      }
      if (ImGui::MenuItem("Load", "Ctrl+O")) {
        out.requestLoad = true;
      }
      if (ImGui::MenuItem("Save As...")) {
        OpenSaveAsModal(state);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
        out.requestQuit = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
        out.requestUndo = true;
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
        out.requestRedo = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Palette", nullptr, &state.showPalette);
      ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
      ImGui::MenuItem("Log", nullptr, &state.showLog);
      ImGui::Separator();
      ImGui::MenuItem("Show Grid", nullptr, &state.showGrid);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        state.openAboutModal = true;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
  }

  ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGuiID dockspaceId = ImGui::GetID("DockSpaceRoot");
  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockFlags);
  ImGui::End();

  ImGui::Begin("Editor");
  if (ImGui::BeginTabBar("EditorTabs")) {
    if (ImGui::BeginTabItem("Saves")) {
      ImGui::Text("Current: %s", state.currentMapPath.c_str());
      if (ImGui::Button("Save")) {
        out.requestSave = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Load")) {
        out.requestLoad = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Save As...")) {
        OpenSaveAsModal(state);
      }
      ImGui::Separator();
      ImGui::Text("Recent");
      if (state.recentFiles.empty()) {
        ImGui::TextDisabled("No recent files.");
      } else {
        for (const std::string& path : state.recentFiles) {
          if (ImGui::Selectable(path.c_str())) {
            out.requestLoad = true;
            out.loadPath = path;
          }
        }
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Tiles")) {
      ImGui::Text("Atlas: %s", editor.atlas.path.c_str());
      ImGui::InputInt("Tile Width", &editor.atlas.tileW);
      ImGui::InputInt("Tile Height", &editor.atlas.tileH);
      ImGui::InputInt("Columns", &editor.atlas.cols);
      ImGui::InputInt("Rows", &editor.atlas.rows);
      if (editor.atlas.tileW < 1) editor.atlas.tileW = 1;
      if (editor.atlas.tileH < 1) editor.atlas.tileH = 1;
      if (editor.atlas.cols < 1) editor.atlas.cols = 1;
      if (editor.atlas.rows < 1) editor.atlas.rows = 1;
      if (ImGui::Button("Reload Atlas")) {
        out.requestReloadAtlas = true;
      }
      ImGui::Separator();
      DrawTileButtons(editor, atlasTexture);

      const int cols = std::max(1, editor.atlas.cols);
      const int rows = std::max(1, editor.atlas.rows);
      const int maxIndex = cols * rows;
      if (editor.currentTileIndex > maxIndex) {
        editor.currentTileIndex = maxIndex;
      }
      if (editor.currentTileIndex < 1) {
        editor.currentTileIndex = 1;
      }

      ImGui::Text("Preview:");
      if (atlasTexture.IsFallback()) {
        ImVec4 color = TileFallbackColor(editor.currentTileIndex);
        ImGui::ColorButton("##preview", color, ImGuiColorEditFlags_NoTooltip, ImVec2(64.0f, 64.0f));
      } else {
        ImVec2 uv0{};
        ImVec2 uv1{};
        if (ComputeAtlasUV(editor.atlas, editor.currentTileIndex, uv0, uv1)) {
          ImTextureID textureId = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(atlasTexture.GetId()));
          ImGui::Image(textureId, ImVec2(64.0f, 64.0f), uv0, uv1);
        }
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Settings")) {
      ImGui::Checkbox("Show Grid", &state.showGrid);
      ImGui::Checkbox("Show FPS", &state.showFps);
      if (ImGui::Checkbox("VSync", &state.vsyncEnabled)) {
        state.vsyncDirty = true;
      }
      if (state.showFps) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("About")) {
      ImGui::TextWrapped("Axiom Tile Editor - a compact tool for painting and editing tile maps.");
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }
  ImGui::End();

  if (state.showPalette) {
    DrawTilePalette(editor, atlasTexture);
  }
  if (state.showInspector) {
    DrawInspector(editor, camera, hover);
  }
  if (state.showLog) {
    DrawLog(log);
  }

  if (state.openSaveAsModal) {
    ImGui::OpenPopup("Save As");
    state.openSaveAsModal = false;
  }
  if (ImGui::BeginPopupModal("Save As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Path", state.saveAsBuffer, sizeof(state.saveAsBuffer));
    if (ImGui::Button("Save")) {
      out.requestSaveAs = true;
      out.saveAsPath = state.saveAsBuffer;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (state.openAboutModal) {
    ImGui::OpenPopup("About");
    state.openAboutModal = false;
  }
  if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("Axiom Tile Editor lets you paint tiles, manage maps, and iterate quickly.");
    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  return out;
}

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

void DrawTilePalette(EditorState& state, const Texture& atlasTexture) {
  ImGui::Begin("Tile Palette");
  DrawTileButtons(state, atlasTexture);
  ImGui::End();
}

void DrawInspector(const EditorState& state, const OrthoCamera& camera, const HoverInfo& hover) {
  ImGui::Begin("Inspector");

  if (hover.hasHover) {
    ImGui::Text("Hover: (%d, %d)", hover.cell.x, hover.cell.y);
  } else {
    ImGui::Text("Hover: none");
  }

  ImGui::Text("Tile Index: %d", state.currentTileIndex);
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
