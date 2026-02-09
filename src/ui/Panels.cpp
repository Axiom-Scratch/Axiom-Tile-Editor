#include "ui/Panels.h"

#include "render/Framebuffer.h"
#include "render/Texture.h"

#include <imgui.h>
#ifdef IMGUI_HAS_DOCK
#include <imgui_internal.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace te::ui {

namespace {

constexpr const char* kEditorConfigPath = "assets/config/editor.json";

ImTextureID ToImTextureID(const Texture& texture) {
  return static_cast<ImTextureID>(static_cast<intptr_t>(texture.GetId()));
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

bool ParseIntAfterKey(const std::string& text, const std::string& key, int& outValue) {
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find(':', pos);
  if (pos == std::string::npos) {
    return false;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  size_t end = pos;
  while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) || text[end] == '-')) {
    ++end;
  }
  if (end == pos) {
    return false;
  }
  try {
    outValue = std::stoi(text.substr(pos, end - pos));
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseFloatAfterKey(const std::string& text, const std::string& key, float& outValue) {
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find(':', pos);
  if (pos == std::string::npos) {
    return false;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  size_t end = pos;
  while (end < text.size() &&
         (std::isdigit(static_cast<unsigned char>(text[end])) || text[end] == '-' || text[end] == '.')) {
    ++end;
  }
  if (end == pos) {
    return false;
  }
  try {
    outValue = std::stof(text.substr(pos, end - pos));
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseStringAfterKey(const std::string& text, const std::string& key, std::string& outValue) {
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find(':', pos);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find('"', pos);
  if (pos == std::string::npos) {
    return false;
  }
  ++pos;
  std::string value;
  bool escape = false;
  for (; pos < text.size(); ++pos) {
    char c = text[pos];
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
  outValue = UnescapeJson(value);
  return true;
}

std::vector<std::string> ExtractJsonArray(const std::string& text, const std::string& key) {
  std::vector<std::string> result;
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return result;
  }
  pos = text.find('[', pos);
  if (pos == std::string::npos) {
    return result;
  }
  size_t end = text.find(']', pos);
  if (end == std::string::npos) {
    return result;
  }
  bool inString = false;
  bool escape = false;
  std::string current;
  for (size_t i = pos + 1; i < end; ++i) {
    char c = text[i];
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

const char* ThemePresetLabel(ThemePreset preset) {
  switch (preset) {
    case ThemePreset::Dark:
      return "Dark";
    case ThemePreset::TrueDark:
      return "TrueDark";
    case ThemePreset::UnityDark:
      return "UnityDark";
    case ThemePreset::Light:
      return "Light";
    default:
      return "TrueDark";
  }
}

ThemePreset ParseThemePreset(const std::string& value, ThemePreset fallback) {
  if (value == "Dark") return ThemePreset::Dark;
  if (value == "TrueDark") return ThemePreset::TrueDark;
  if (value == "UnityDark") return ThemePreset::UnityDark;
  if (value == "Light") return ThemePreset::Light;
  return fallback;
}

ThemeSettings DefaultThemeSettings(ThemePreset preset) {
  ThemeSettings settings{};
  settings.preset = preset;
  if (preset == ThemePreset::TrueDark) {
    settings.globalAlpha = 1.0f;
    settings.windowBgAlpha = 0.98f;
    settings.frameBgAlpha = 0.95f;
    settings.popupBgAlpha = 0.98f;
    settings.rounding = 4.0f;
  } else {
    settings.globalAlpha = 0.95f;
    settings.windowBgAlpha = 0.75f;
    settings.frameBgAlpha = 0.85f;
    settings.popupBgAlpha = 0.9f;
    settings.rounding = 4.0f;
  }
  settings.boostContrast = false;
  return settings;
}

void ApplyDefaults(EditorUIState& state) {
  state.currentMapPath = "assets/maps/map.json";
  state.lastAtlas.path = "assets/textures/atlas.png";
  state.lastAtlas.tileW = 32;
  state.lastAtlas.tileH = 32;
  state.lastAtlas.cols = 0;
  state.lastAtlas.rows = 0;
  state.showSettings = true;
  state.theme = DefaultThemeSettings(ThemePreset::TrueDark);
  state.themeDirty = true;
}

void EnsureBuffer(char* buffer, size_t size, const std::string& value) {
  std::snprintf(buffer, size, "%s", value.c_str());
}

void SaveEditorConfigInternal(const EditorUIState& state) {
  std::filesystem::create_directories("assets/config");

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
  file << "  ],\n";
  file << "  \"windowWidth\": " << state.windowWidth << ",\n";
  file << "  \"windowHeight\": " << state.windowHeight << ",\n";
  file << "  \"atlasPath\": \"" << EscapeJson(state.lastAtlas.path) << "\",\n";
  file << "  \"atlasTileW\": " << state.lastAtlas.tileW << ",\n";
  file << "  \"atlasTileH\": " << state.lastAtlas.tileH << ",\n";
  file << "  \"atlasCols\": " << state.lastAtlas.cols << ",\n";
  file << "  \"atlasRows\": " << state.lastAtlas.rows << ",\n";
  file << "  \"themePreset\": \"" << ThemePresetLabel(state.theme.preset) << "\",\n";
  file << "  \"themeGlobalAlpha\": " << state.theme.globalAlpha << ",\n";
  file << "  \"themeWindowBgAlpha\": " << state.theme.windowBgAlpha << ",\n";
  file << "  \"themeFrameBgAlpha\": " << state.theme.frameBgAlpha << ",\n";
  file << "  \"themePopupBgAlpha\": " << state.theme.popupBgAlpha << ",\n";
  file << "  \"themeRounding\": " << state.theme.rounding << ",\n";
  file << "  \"themeBoostContrast\": " << (state.theme.boostContrast ? 1 : 0) << "\n";
  file << "}\n";
}

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

void DrawTileGrid(EditorState& editor, const Texture& atlasTexture, float buttonSize) {
  const int cols = std::max(1, editor.atlas.cols);
  const int rows = std::max(1, editor.atlas.rows);
  const int total = cols * rows;

  ImTextureID textureId = ToImTextureID(atlasTexture);
#if IMGUI_VERSION_NUM >= 19200
  ImTextureRef textureRef(textureId);
#endif
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
        if (ComputeAtlasUV(editor.atlas, tileIndex, uv0, uv1)) {
          char buttonId[32];
          std::snprintf(buttonId, sizeof(buttonId), "tile##%d", tileIndex);
#if IMGUI_VERSION_NUM >= 19200
          clicked = ImGui::ImageButton(buttonId, textureRef, ImVec2(buttonSize, buttonSize), uv0, uv1);
#else
          clicked = ImGui::ImageButton(buttonId, textureId, ImVec2(buttonSize, buttonSize), uv0, uv1);
#endif
        } else {
          clicked = ImGui::Button("?", ImVec2(buttonSize, buttonSize));
        }
      }
      if (clicked) {
        editor.currentTileIndex = tileIndex;
      }
      if (col + 1 < cols) {
        ImGui::SameLine();
      }
      ImGui::PopID();
    }
  }
}

void DrawMenuBar(EditorUIState& state, EditorUIOutput& out) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
      out.requestSave = true;
    }
    if (ImGui::MenuItem("Load", "Ctrl+O")) {
      out.requestLoad = true;
    }
    if (ImGui::MenuItem("Save As...")) {
      state.openSaveAsModal = true;
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
    ImGui::MenuItem("Hierarchy", nullptr, &state.showHierarchy);
    ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
    ImGui::MenuItem("Settings", nullptr, &state.showSettings);
    ImGui::MenuItem("Project", nullptr, &state.showProject);
    ImGui::MenuItem("Console", nullptr, &state.showConsole);
    ImGui::MenuItem("Tile Palette", nullptr, &state.showTilePalette);
    ImGui::Separator();
    ImGui::MenuItem("Grid", nullptr, &state.showGrid);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("About")) {
      state.openAboutModal = true;
    }
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void DrawToolbar(EditorUIState& state, EditorUIOutput& out, EditorState& editor, const Texture& atlasTexture) {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (!ImGui::Begin("Toolbar", nullptr, flags)) {
    ImGui::End();
    return;
  }

  if (ImGui::RadioButton("Paint", editor.currentTool == Tool::Paint)) {
    editor.currentTool = Tool::Paint;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Erase", editor.currentTool == Tool::Erase)) {
    editor.currentTool = Tool::Erase;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Pan", editor.currentTool == Tool::Pan)) {
    editor.currentTool = Tool::Pan;
  }

  ImGui::SameLine();
  ImGui::Separator();
  ImGui::SameLine();

  if (ImGui::Button("Save")) {
    out.requestSave = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    out.requestLoad = true;
  }

  ImGui::SameLine();
  ImGui::Checkbox("Snap", &state.snapEnabled);

  ImGui::SameLine();
  ImGui::Text("Tile");
  ImGui::SameLine();

  if (atlasTexture.IsFallback()) {
    ImVec4 color = TileFallbackColor(editor.currentTileIndex);
    ImGui::ColorButton("##toolbar_tile", color, ImGuiColorEditFlags_NoTooltip, ImVec2(24.0f, 24.0f));
  } else {
    ImVec2 uv0{};
    ImVec2 uv1{};
    if (ComputeAtlasUV(editor.atlas, editor.currentTileIndex, uv0, uv1)) {
      ImTextureID textureId = ToImTextureID(atlasTexture);
#if IMGUI_VERSION_NUM >= 19200
      ImGui::Image(ImTextureRef(textureId), ImVec2(24.0f, 24.0f), uv0, uv1);
#else
      ImGui::Image(textureId, ImVec2(24.0f, 24.0f), uv0, uv1);
#endif
    }
  }

  ImGui::End();
}

void DrawSceneView(EditorUIState& state, EditorUIOutput& out, Framebuffer& framebuffer) {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoBackground;
  if (!ImGui::Begin("Scene View", nullptr, flags)) {
    state.sceneHovered = false;
    state.sceneRect = {};
    ImGui::End();
    return;
  }

  if (ImGui::Button("Focus")) {
    out.requestFocus = true;
  }
  ImGui::SameLine();
  ImGui::Checkbox("Grid", &state.showGrid);

  ImVec2 sceneSize = ImGui::GetContentRegionAvail();
  if (sceneSize.x < 1.0f) sceneSize.x = 1.0f;
  if (sceneSize.y < 1.0f) sceneSize.y = 1.0f;

  const ImGuiIO& io = ImGui::GetIO();
  const int fbWidth = std::max(1, static_cast<int>(sceneSize.x * io.DisplayFramebufferScale.x));
  const int fbHeight = std::max(1, static_cast<int>(sceneSize.y * io.DisplayFramebufferScale.y));
  framebuffer.Resize(fbWidth, fbHeight);

  ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(framebuffer.GetColorTexture()));
#if IMGUI_VERSION_NUM >= 19200
  ImGui::Image(ImTextureRef(texId), sceneSize, ImVec2(0, 1), ImVec2(1, 0));
#else
  ImGui::Image(texId, sceneSize, ImVec2(0, 1), ImVec2(1, 0));
#endif
  state.sceneHovered = ImGui::IsItemHovered();
  ImVec2 rectMin = ImGui::GetItemRectMin();
  ImVec2 rectMax = ImGui::GetItemRectMax();
  state.sceneRect = {rectMin.x, rectMin.y, rectMax.x - rectMin.x, rectMax.y - rectMin.y};

  ImGui::End();
}

void DrawHierarchy(EditorState& editor) {
  if (!ImGui::Begin("Hierarchy")) {
    ImGui::End();
    return;
  }

  const bool rootSelected = editor.selectedLayer < 0;
  if (ImGui::Selectable("Tilemap", rootSelected)) {
    editor.selectedLayer = -1;
  }

  if (ImGui::TreeNodeEx("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (size_t i = 0; i < editor.layers.size(); ++i) {
      const bool selected = editor.selectedLayer == static_cast<int>(i);
      if (ImGui::Selectable(editor.layers[i].name.c_str(), selected)) {
        editor.selectedLayer = static_cast<int>(i);
      }
    }
    ImGui::TreePop();
  }

  ImGui::End();
}

void DrawInspector(EditorUIState& state, EditorUIOutput& out, EditorState& editor) {
  if (!ImGui::Begin("Inspector")) {
    ImGui::End();
    return;
  }

  if (editor.selectedLayer >= 0 && editor.selectedLayer < static_cast<int>(editor.layers.size())) {
    Layer& layer = editor.layers[static_cast<size_t>(editor.selectedLayer)];
    if (state.lastLayerSelection != editor.selectedLayer) {
      EnsureBuffer(state.layerNameBuffer, sizeof(state.layerNameBuffer), layer.name);
      state.lastLayerSelection = editor.selectedLayer;
    }
    if (ImGui::InputText("Name", state.layerNameBuffer, sizeof(state.layerNameBuffer))) {
      layer.name = state.layerNameBuffer;
    }
    ImGui::Checkbox("Visible", &layer.visible);
    ImGui::Checkbox("Locked", &layer.locked);
    ImGui::SliderFloat("Opacity", &layer.opacity, 0.0f, 1.0f, "%.2f");
  } else {
    if (state.pendingMapWidth <= 0) {
      state.pendingMapWidth = editor.tileMap.GetWidth();
    }
    if (state.pendingMapHeight <= 0) {
      state.pendingMapHeight = editor.tileMap.GetHeight();
    }

    ImGui::Text("Map");
    ImGui::InputInt("Width", &state.pendingMapWidth);
    ImGui::InputInt("Height", &state.pendingMapHeight);
    ImGui::Text("Tile Size: %d", editor.tileMap.GetTileSize());
    if (state.pendingMapWidth < 1) state.pendingMapWidth = 1;
    if (state.pendingMapHeight < 1) state.pendingMapHeight = 1;

    if (ImGui::Button("Apply")) {
      state.openResizeModal = true;
    }

    ImGui::Separator();
    ImGui::Text("Atlas");
    if (state.atlasPathBuffer[0] == '\0') {
      EnsureBuffer(state.atlasPathBuffer, sizeof(state.atlasPathBuffer), editor.atlas.path);
    }
    ImGui::InputText("Path", state.atlasPathBuffer, sizeof(state.atlasPathBuffer));
    ImGui::InputInt("Tile W", &editor.atlas.tileW);
    ImGui::InputInt("Tile H", &editor.atlas.tileH);
    ImGui::InputInt("Cols", &editor.atlas.cols);
    ImGui::InputInt("Rows", &editor.atlas.rows);
    if (editor.atlas.tileW < 1) editor.atlas.tileW = 1;
    if (editor.atlas.tileH < 1) editor.atlas.tileH = 1;
    if (editor.atlas.cols < 1) editor.atlas.cols = 1;
    if (editor.atlas.rows < 1) editor.atlas.rows = 1;

    if (ImGui::Button("Reload Atlas")) {
      out.atlasPath = state.atlasPathBuffer;
      out.requestReloadAtlas = true;
    }
  }

  ImGui::End();
}

void DrawSettings(EditorUIState& state) {
  if (!ImGui::Begin("Settings")) {
    ImGui::End();
    return;
  }

  ThemeSettings& theme = state.theme;
  int presetIndex = 0;
  if (theme.preset == ThemePreset::TrueDark) {
    presetIndex = 1;
  } else if (theme.preset == ThemePreset::UnityDark) {
    presetIndex = 2;
  } else if (theme.preset == ThemePreset::Light) {
    presetIndex = 3;
  }

  bool changed = false;
  if (ImGui::Combo("Theme Preset", &presetIndex, "Dark\0TrueDark\0UnityDark\0Light\0")) {
    ThemePreset newPreset = (presetIndex == 0) ? ThemePreset::Dark :
                            (presetIndex == 1) ? ThemePreset::TrueDark :
                            (presetIndex == 2) ? ThemePreset::UnityDark :
                                                 ThemePreset::Light;
    if (newPreset != theme.preset) {
      bool boost = theme.boostContrast;
      theme = DefaultThemeSettings(newPreset);
      theme.boostContrast = boost;
      changed = true;
    }
  }

  changed |= ImGui::SliderFloat("UI Opacity (Global)", &theme.globalAlpha, 0.6f, 1.0f, "%.2f");
  changed |= ImGui::SliderFloat("Panel Opacity (WindowBg)", &theme.windowBgAlpha, 0.85f, 1.0f, "%.2f");
  changed |= ImGui::SliderFloat("Widget Opacity (FrameBg)", &theme.frameBgAlpha, 0.80f, 1.0f, "%.2f");
  changed |= ImGui::SliderFloat("Rounding", &theme.rounding, 0.0f, 8.0f, "%.1f");
  changed |= ImGui::Checkbox("Boost Contrast", &theme.boostContrast);

  if (ImGui::Button("Make Opaque (Dark)")) {
    theme.globalAlpha = 1.0f;
    theme.windowBgAlpha = 0.98f;
    theme.frameBgAlpha = 0.95f;
    theme.popupBgAlpha = 0.98f;
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Transparent UI (for viewing scene)")) {
    theme.windowBgAlpha = 0.85f;
    theme.frameBgAlpha = 0.85f;
    theme.popupBgAlpha = 0.85f;
    changed = true;
  }

  if (ImGui::Button("Reset Theme")) {
    theme = DefaultThemeSettings(theme.preset);
    changed = true;
  }

  if (changed) {
    theme.popupBgAlpha = theme.windowBgAlpha;
    state.themeDirty = true;
  }

  ImGui::End();
}

void DrawProjectDirectory(const std::filesystem::path& root,
                          const char* label,
                          bool treatAsMap,
                          bool treatAsTexture,
                          bool hasUnsavedChanges,
                          EditorUIState& state,
                          EditorUIOutput& out) {
  if (!std::filesystem::exists(root)) {
    ImGui::TextDisabled("%s (missing)", label);
    return;
  }

  if (!ImGui::TreeNode(label)) {
    return;
  }

  std::vector<std::filesystem::directory_entry> entries;
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    entries.push_back(entry);
  }

  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
    return a.path().filename().string() < b.path().filename().string();
  });

  for (const auto& entry : entries) {
    const std::filesystem::path& path = entry.path();
    const std::string name = path.filename().string();
    if (entry.is_directory()) {
      DrawProjectDirectory(path, name.c_str(), treatAsMap, treatAsTexture, hasUnsavedChanges, state, out);
      continue;
    }

    if (ImGui::Selectable(name.c_str())) {
      const std::string relPath = path.generic_string();
      if (treatAsMap) {
        if (hasUnsavedChanges) {
          state.pendingLoadPath = relPath;
          state.openUnsavedModal = true;
        } else {
          out.requestLoad = true;
          out.loadPath = relPath;
        }
      } else if (treatAsTexture) {
        out.atlasPath = relPath;
        out.requestReloadAtlas = true;
      }
    }
  }

  ImGui::TreePop();
}

void DrawProject(EditorUIState& state, EditorUIOutput& out, const EditorState& editor) {
  if (!ImGui::Begin("Project")) {
    ImGui::End();
    return;
  }

  DrawProjectDirectory("assets/maps", "assets/maps", true, false, editor.hasUnsavedChanges, state, out);
  DrawProjectDirectory("assets/textures", "assets/textures", false, true, editor.hasUnsavedChanges, state, out);
  if (std::filesystem::exists("assets/shaders")) {
    DrawProjectDirectory("assets/shaders", "assets/shaders", false, false, editor.hasUnsavedChanges, state, out);
  }

  ImGui::End();
}

void DrawConsole(EditorUIState& state, Log& log) {
  if (!ImGui::Begin("Console")) {
    ImGui::End();
    return;
  }

  ImGui::Checkbox("Info", &state.filterInfo);
  ImGui::SameLine();
  ImGui::Checkbox("Warn", &state.filterWarn);
  ImGui::SameLine();
  ImGui::Checkbox("Error", &state.filterError);
  ImGui::InputText("Filter", state.consoleFilter, sizeof(state.consoleFilter));

  const std::string filterText = state.consoleFilter;
  const auto& lines = log.GetLines();
  for (const std::string& line : lines) {
    bool isInfo = line.rfind("[Info]", 0) == 0;
    bool isWarn = line.rfind("[Warn]", 0) == 0;
    bool isError = line.rfind("[Error]", 0) == 0;

    if ((isInfo && !state.filterInfo) || (isWarn && !state.filterWarn) || (isError && !state.filterError)) {
      continue;
    }

    if (!filterText.empty() && line.find(filterText) == std::string::npos) {
      continue;
    }

    ImGui::TextUnformatted(line.c_str());
  }

  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::End();
}

void DrawTilePalette(EditorState& editor, const Texture& atlasTexture) {
  if (!ImGui::Begin("Tile Palette")) {
    ImGui::End();
    return;
  }

  DrawTileGrid(editor, atlasTexture, 36.0f);
  ImGui::Text("Current: %d", editor.currentTileIndex);

  ImGui::End();
}

void DrawResizeModal(EditorUIState& state, EditorUIOutput& out) {
  if (state.openResizeModal) {
    ImGui::OpenPopup("Resize Map");
    state.openResizeModal = false;
  }

  if (ImGui::BeginPopupModal("Resize Map", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Resize the map to %d x %d?", state.pendingMapWidth, state.pendingMapHeight);
    if (ImGui::Button("Apply")) {
      out.requestResizeMap = true;
      out.resizeWidth = state.pendingMapWidth;
      out.resizeHeight = state.pendingMapHeight;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawSaveAsModal(EditorUIState& state, EditorUIOutput& out) {
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
}

void DrawAboutModal(EditorUIState& state) {
  if (state.openAboutModal) {
    ImGui::OpenPopup("About");
    state.openAboutModal = false;
  }

  if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("Axiom Tile Editor - Unity-inspired layout using ImGui docking.");
    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawUnsavedModal(EditorUIState& state, EditorUIOutput& out) {
  if (state.openUnsavedModal) {
    ImGui::OpenPopup("Unsaved Changes");
    state.openUnsavedModal = false;
  }

  if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("You have unsaved changes. Load anyway?");
    if (ImGui::Button("Load")) {
      out.requestLoad = true;
      out.loadPath = state.pendingLoadPath;
      state.pendingLoadPath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      state.pendingLoadPath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void BuildDockSpace(EditorUIState& state) {
#ifdef IMGUI_HAS_DOCK
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGui::DockSpaceOverViewport(0, viewport, dockFlags);

  if (state.dockInitialized) {
    return;
  }

  const char* iniPath = ImGui::GetIO().IniFilename;
  const bool hasIni = iniPath && std::filesystem::exists(iniPath);
  if (!hasIni) {
    ImGuiID dockspaceId = viewport->ID;
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, dockFlags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockToolbar = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Up, 0.08f, nullptr, &dockMain);
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f, nullptr, &dockMain);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.22f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.26f, nullptr, &dockMain);

    ImGui::DockBuilderDockWindow("Toolbar", dockToolbar);
    ImGui::DockBuilderDockWindow("Scene View", dockMain);
    ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
    ImGui::DockBuilderDockWindow("Tile Palette", dockLeft);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    ImGui::DockBuilderDockWindow("Settings", dockRight);
    ImGui::DockBuilderDockWindow("Project", dockBottom);
    ImGui::DockBuilderDockWindow("Console", dockBottom);

    ImGui::DockBuilderFinish(dockspaceId);
  }

  state.dockInitialized = true;
#else
  (void)state;
#endif
}

} // namespace

void LoadEditorConfig(EditorUIState& state) {
  state = {};
  ApplyDefaults(state);

  std::ifstream file(kEditorConfigPath);
  if (!file) {
    return;
  }

  std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  ParseStringAfterKey(text, "current", state.currentMapPath);
  state.recentFiles = ExtractJsonArray(text, "recent");
  if (state.recentFiles.size() > 10) {
    state.recentFiles.resize(10);
  }

  ParseIntAfterKey(text, "windowWidth", state.windowWidth);
  ParseIntAfterKey(text, "windowHeight", state.windowHeight);

  ParseStringAfterKey(text, "atlasPath", state.lastAtlas.path);
  ParseIntAfterKey(text, "atlasTileW", state.lastAtlas.tileW);
  ParseIntAfterKey(text, "atlasTileH", state.lastAtlas.tileH);
  ParseIntAfterKey(text, "atlasCols", state.lastAtlas.cols);
  ParseIntAfterKey(text, "atlasRows", state.lastAtlas.rows);

  std::string themePreset;
  const bool hasThemePreset = ParseStringAfterKey(text, "themePreset", themePreset);
  if (hasThemePreset) {
    ThemePreset parsed = ParseThemePreset(themePreset, ThemePreset::TrueDark);
    state.theme = DefaultThemeSettings(parsed);
    ParseFloatAfterKey(text, "themeGlobalAlpha", state.theme.globalAlpha);
    ParseFloatAfterKey(text, "themeWindowBgAlpha", state.theme.windowBgAlpha);
    ParseFloatAfterKey(text, "themeFrameBgAlpha", state.theme.frameBgAlpha);
    ParseFloatAfterKey(text, "themePopupBgAlpha", state.theme.popupBgAlpha);
    ParseFloatAfterKey(text, "themeRounding", state.theme.rounding);
    int boostContrast = state.theme.boostContrast ? 1 : 0;
    if (ParseIntAfterKey(text, "themeBoostContrast", boostContrast)) {
      state.theme.boostContrast = boostContrast != 0;
    }
  } else {
    state.theme = DefaultThemeSettings(ThemePreset::TrueDark);
  }

  if (state.lastAtlas.path.empty()) {
    state.lastAtlas.path = "assets/textures/atlas.png";
  }
  if (state.lastAtlas.tileW <= 0) {
    state.lastAtlas.tileW = 32;
  }
  if (state.lastAtlas.tileH <= 0) {
    state.lastAtlas.tileH = 32;
  }
  if (state.currentMapPath.empty()) {
    state.currentMapPath = "assets/maps/map.json";
  }
  state.themeDirty = true;
}

void SaveEditorConfig(const EditorUIState& state) {
  SaveEditorConfigInternal(state);
}

void AddRecentFile(EditorUIState& state, const std::string& path) {
  if (path.empty()) {
    return;
  }

  state.currentMapPath = path;
  auto it = std::find(state.recentFiles.begin(), state.recentFiles.end(), path);
  if (it != state.recentFiles.end()) {
    state.recentFiles.erase(it);
  }
  state.recentFiles.insert(state.recentFiles.begin(), path);
  if (state.recentFiles.size() > 10) {
    state.recentFiles.resize(10);
  }
  SaveEditorConfigInternal(state);
}

const std::string& GetCurrentMapPath(const EditorUIState& state) {
  return state.currentMapPath;
}

EditorUIOutput DrawEditorUI(EditorUIState& state,
                            EditorState& editor,
                            Log& log,
                            const Texture& atlasTexture,
                            Framebuffer& sceneFramebuffer) {
  EditorUIOutput out{};

  if (state.themeDirty) {
    ApplyTheme(state.theme);
    state.themeDirty = false;
  }

  BuildDockSpace(state);
  DrawMenuBar(state, out);
  DrawToolbar(state, out, editor, atlasTexture);
  DrawSceneView(state, out, sceneFramebuffer);

  if (state.showHierarchy) {
    DrawHierarchy(editor);
  }
  if (state.showInspector) {
    DrawInspector(state, out, editor);
  }
  if (state.showSettings) {
    DrawSettings(state);
  }
  if (state.showProject) {
    DrawProject(state, out, editor);
  }
  if (state.showConsole) {
    DrawConsole(state, log);
  }
  if (state.showTilePalette) {
    DrawTilePalette(editor, atlasTexture);
  }

  DrawSaveAsModal(state, out);
  DrawAboutModal(state);
  DrawResizeModal(state, out);
  DrawUnsavedModal(state, out);

  return out;
}

void DrawSceneOverlay(const EditorUIState& state,
                      float fps,
                      float zoom,
                      bool hasHover,
                      Vec2i hoverCell,
                      int tileIndex) {
  if (state.sceneRect.width <= 1.0f || state.sceneRect.height <= 1.0f) {
    return;
  }

  char buffer[256];
  if (hasHover) {
    std::snprintf(buffer, sizeof(buffer), "FPS: %.1f\nZoom: %.2f\nHover: (%d, %d)\nTile: %d",
                  fps, zoom, hoverCell.x, hoverCell.y, tileIndex);
  } else {
    std::snprintf(buffer, sizeof(buffer), "FPS: %.1f\nZoom: %.2f\nHover: none\nTile: %d",
                  fps, zoom, tileIndex);
  }

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  ImVec2 pos(state.sceneRect.x + 8.0f, state.sceneRect.y + 8.0f);
  ImVec2 textSize = ImGui::CalcTextSize(buffer);
  ImVec2 padding(6.0f, 6.0f);
  ImVec2 rectMin(pos.x - padding.x, pos.y - padding.y);
  ImVec2 rectMax(pos.x + textSize.x + padding.x, pos.y + textSize.y + padding.y);

  drawList->PushClipRect(ImVec2(state.sceneRect.x, state.sceneRect.y),
                         ImVec2(state.sceneRect.x + state.sceneRect.width,
                                state.sceneRect.y + state.sceneRect.height),
                         true);
  drawList->AddRectFilled(rectMin, rectMax, IM_COL32(0, 0, 0, 150));
  drawList->AddText(pos, IM_COL32(255, 255, 255, 230), buffer);
  drawList->PopClipRect();
}

} // namespace te::ui
