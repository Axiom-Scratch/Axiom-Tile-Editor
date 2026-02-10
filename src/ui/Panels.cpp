#include "ui/Panels.h"

#include "render/Framebuffer.h"
#include "render/Texture.h"

#include <imgui.h>
#ifdef IMGUI_HAS_DOCK
#include <imgui_internal.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
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

bool EndsWith(const std::string& value, const std::string& suffix) {
  if (value.size() < suffix.size()) {
    return false;
  }
  return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string EnsureJsonExtension(const std::string& path) {
  if (path.empty()) {
    return path;
  }
  if (EndsWith(path, ".json")) {
    return path;
  }
  return path + ".json";
}

std::string FormatTimestamp(const std::filesystem::file_time_type& time) {
  using namespace std::chrono;
  const auto now = std::filesystem::file_time_type::clock::now();
  const auto systemNow = system_clock::now();
  const auto adjusted = time - now + systemNow;
  const auto timePoint = time_point_cast<system_clock::duration>(adjusted);
  std::time_t stamp = system_clock::to_time_t(timePoint);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &stamp);
#else
  localtime_r(&stamp, &tm);
#endif
  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
  return buffer;
}

std::vector<std::filesystem::path> CollectFiles(const std::filesystem::path& root,
                                                const std::function<bool(const std::filesystem::path&)>& filter) {
  std::vector<std::filesystem::path> results;
  if (!std::filesystem::exists(root)) {
    return results;
  }
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (filter(entry.path())) {
      results.push_back(entry.path());
    }
  }
  std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) { return a.filename().string() < b.filename().string(); });
  return results;
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

const char* ToolLabel(Tool tool) {
  switch (tool) {
    case Tool::Paint:
      return "Paint";
    case Tool::Erase:
      return "Erase";
    case Tool::Rect:
      return "Rect";
    case Tool::Fill:
      return "Fill";
    case Tool::Line:
      return "Line";
    case Tool::Stamp:
      return "Stamp";
    case Tool::Pick:
      return "Pick";
    case Tool::Select:
      return "Select";
    case Tool::Move:
      return "Move";
    case Tool::Pan:
      return "Pan";
    default:
      return "Unknown";
  }
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
  state.autosaveEnabled = false;
  state.autosaveInterval = 60.0f;
  state.autosavePath = "assets/autosave/autosave.json";
  state.gridCellSize = 0.0f;
  state.gridMajorStep = 8;
  state.gridColor = {0.15f, 0.15f, 0.18f, 1.0f};
  state.gridAlpha = 0.7f;
  state.invertZoom = false;
  state.panSpeed = 1.0f;
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
  file << "  \"themeBoostContrast\": " << (state.theme.boostContrast ? 1 : 0) << ",\n";
  file << "  \"autosaveEnabled\": " << (state.autosaveEnabled ? 1 : 0) << ",\n";
  file << "  \"autosaveInterval\": " << state.autosaveInterval << ",\n";
  file << "  \"autosavePath\": \"" << EscapeJson(state.autosavePath) << "\",\n";
  file << "  \"gridCellSize\": " << state.gridCellSize << ",\n";
  file << "  \"gridMajorStep\": " << state.gridMajorStep << ",\n";
  file << "  \"gridColorR\": " << state.gridColor.r << ",\n";
  file << "  \"gridColorG\": " << state.gridColor.g << ",\n";
  file << "  \"gridColorB\": " << state.gridColor.b << ",\n";
  file << "  \"gridAlpha\": " << state.gridAlpha << ",\n";
  file << "  \"invertZoom\": " << (state.invertZoom ? 1 : 0) << ",\n";
  file << "  \"panSpeed\": " << state.panSpeed << "\n";
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

bool BeginInspectorTable() {
  if (!ImGui::BeginTable("InspectorTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
    return false;
  }
  ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
  return true;
}

void EndInspectorTable() {
  ImGui::EndTable();
}

void InspectorRowLabel(const char* label) {
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::TableSetColumnIndex(1);
}

void IntStepper(const char* id, int* value, int step, int minV, int maxV) {
  const float frameHeight = ImGui::GetFrameHeight();
  const float buttonSize = frameHeight * 1.2f;
  ImGui::PushID(id);
  if (ImGui::BeginTable("##stepper", 3, ImGuiTableFlags_SizingStretchSame)) {
    ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Minus", ImGuiTableColumnFlags_WidthFixed, buttonSize);
    ImGui::TableSetupColumn("Plus", ImGuiTableColumnFlags_WidthFixed, buttonSize);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##value", value, 0, 0);

    ImGui::TableSetColumnIndex(1);
    if (ImGui::Button("-", ImVec2(buttonSize, frameHeight))) {
      *value -= step;
    }

    ImGui::TableSetColumnIndex(2);
    if (ImGui::Button("+", ImVec2(buttonSize, frameHeight))) {
      *value += step;
    }

    ImGui::EndTable();
  }

  if (*value < minV) *value = minV;
  if (*value > maxV) *value = maxV;
  ImGui::PopID();
}

bool BeginInspectorSection(const char* label, bool defaultOpen, bool* resetClicked) {
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
  if (defaultOpen) {
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
  }

  ImGui::PushID(label);
  const bool open = ImGui::TreeNodeEx("##section", flags, "%s", label);
  const float buttonWidth = ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.0f;
  ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonWidth);
  if (ImGui::SmallButton("Reset")) {
    if (resetClicked) {
      *resetClicked = true;
    }
  }
  ImGui::PopID();
  return open;
}

std::string ToLowerCopy(const std::string& input) {
  std::string out = input;
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

bool MatchesProjectFilter(const std::string& name, const char* filter) {
  if (!filter || filter[0] == '\0') {
    return true;
  }
  const std::string nameLower = ToLowerCopy(name);
  const std::string filterLower = ToLowerCopy(filter);
  return nameLower.find(filterLower) != std::string::npos;
}

int CountFilesInDirectory(const std::filesystem::path& root) {
  if (!std::filesystem::exists(root)) {
    return 0;
  }
  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(root)) {
    if (entry.is_regular_file()) {
      ++count;
    }
  }
  return count;
}

bool IsPathUnderMaps(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::path root = std::filesystem::weakly_canonical("assets/maps", ec);
  if (ec) {
    return false;
  }
  std::filesystem::path target = std::filesystem::weakly_canonical(path, ec);
  if (ec) {
    return false;
  }
  std::string rootStr = root.generic_string();
  if (!rootStr.empty() && rootStr.back() != '/') {
    rootStr.push_back('/');
  }
  const std::string targetStr = target.generic_string();
  return targetStr.size() >= rootStr.size() && targetStr.compare(0, rootStr.size(), rootStr) == 0;
}

std::filesystem::path MakeUniquePath(const std::filesystem::path& basePath, const std::string& suffix) {
  const std::filesystem::path parent = basePath.parent_path();
  const std::string stem = basePath.stem().string() + suffix;
  const std::string ext = basePath.extension().string();
  for (int i = 0; i < 1000; ++i) {
    std::string name = stem;
    if (i > 0) {
      name += std::to_string(i);
    }
    std::filesystem::path candidate = parent / (name + ext);
    if (!std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return basePath;
}

bool WriteNewMapFile(const std::filesystem::path& path, const EditorState& editor) {
  std::ofstream file(path, std::ios::trunc);
  if (!file) {
    return false;
  }

  const int width = editor.tileMap.GetWidth();
  const int height = editor.tileMap.GetHeight();
  const int tileSize = editor.tileMap.GetTileSize();
  const int total = width * height;

  file << "{\n";
  file << "  \"width\": " << width << ",\n";
  file << "  \"height\": " << height << ",\n";
  file << "  \"tileSize\": " << tileSize << ",\n";
  file << "  \"atlas\": {\n";
  file << "    \"path\": \"" << EscapeJson(editor.atlas.path) << "\",\n";
  file << "    \"tileW\": " << editor.atlas.tileW << ",\n";
  file << "    \"tileH\": " << editor.atlas.tileH << ",\n";
  file << "    \"cols\": " << editor.atlas.cols << ",\n";
  file << "    \"rows\": " << editor.atlas.rows << "\n";
  file << "  },\n";
  file << "  \"data\": [";
  for (int i = 0; i < total; ++i) {
    file << 0;
    if (i + 1 < total) {
      file << ", ";
    }
  }
  file << "]\n";
  file << "}\n";
  return true;
}

std::filesystem::path MakeMapCopyPath(const std::filesystem::path& sourcePath) {
  const std::filesystem::path parent = sourcePath.parent_path();
  const std::string stem = sourcePath.stem().string();
  const std::string ext = sourcePath.extension().string();
  for (int i = 1; i <= 999; ++i) {
    const std::string name = stem + "_copy_" + std::to_string(i) + ext;
    std::filesystem::path candidate = parent / name;
    if (!std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return sourcePath;
}

void DrawMenuBar(EditorUIState& state, EditorUIOutput& out, const EditorState& editor) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  auto queuePending = [&](PendingAction action, const std::string& path) {
    state.pendingAction = action;
    state.pendingLoadPath = path;
    if (action == PendingAction::Quit) {
      state.showConfirmQuit = true;
    } else {
      state.showConfirmOpen = true;
    }
  };

  auto requestNew = [&]() {
    if (editor.hasUnsavedChanges) {
      queuePending(PendingAction::NewMap, "");
    } else {
      out.requestNewMap = true;
    }
  };

  auto requestOpenPicker = [&]() {
    if (editor.hasUnsavedChanges) {
      queuePending(PendingAction::OpenPicker, "");
    } else {
      state.showOpenModal = true;
    }
  };

  auto requestLoadPath = [&](const std::string& path) {
    if (editor.hasUnsavedChanges) {
      queuePending(PendingAction::LoadPath, path);
    } else {
      out.requestLoad = true;
      out.loadPath = path;
    }
  };

  auto requestQuit = [&]() {
    if (editor.hasUnsavedChanges) {
      queuePending(PendingAction::Quit, "");
    } else {
      out.requestQuit = true;
    }
  };

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("New", "Ctrl+N")) {
      requestNew();
    }
    if (ImGui::MenuItem("Open...", "Ctrl+O")) {
      requestOpenPicker();
    }
    if (ImGui::BeginMenu("Open Recent", !state.recentFiles.empty())) {
      for (const std::string& path : state.recentFiles) {
        if (ImGui::MenuItem(path.c_str())) {
          requestLoadPath(path);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
      out.requestSave = true;
    }
    if (ImGui::MenuItem("Save As...")) {
      state.showSaveAs = true;
    }
    if (ImGui::MenuItem("Recover Autosave...")) {
      state.showRecoverAutosave = true;
    }
    if (ImGui::MenuItem("Export CSV")) {
      out.requestExportCsv = true;
    }
    if (ImGui::MenuItem("Import CSV")) {
      out.requestImportCsv = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
      requestQuit();
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Edit")) {
    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor.history.CanUndo())) {
      out.requestUndo = true;
    }
    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor.history.CanRedo())) {
      out.requestRedo = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Create Stamp")) {
      state.openStampModal = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Preferences...")) {
      state.showPreferences = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Grid", nullptr, &state.showGrid);
    if (ImGui::MenuItem("Reset Camera")) {
      out.requestFocus = true;
    }
    if (ImGui::MenuItem("Frame", "F")) {
      out.requestFrame = true;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Window")) {
    if (ImGui::MenuItem("Reset Layout")) {
      state.requestResetLayout = true;
    }
    ImGui::Separator();
    ImGui::MenuItem("Hierarchy", nullptr, &state.showHierarchy);
    ImGui::MenuItem("Scene", nullptr, &state.showScene);
    ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
    ImGui::MenuItem("Palette", nullptr, &state.showTilePalette);
    ImGui::MenuItem("Project", nullptr, &state.showProject);
    ImGui::MenuItem("Console", nullptr, &state.showConsole);
    ImGui::MenuItem("Settings", nullptr, &state.showSettings);
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    if (ImGui::MenuItem("About")) {
      state.showAbout = true;
    }
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void DrawToolbar(EditorUIState& state, EditorUIOutput& out, EditorState& editor, const Texture& atlasTexture) {
  (void)state;
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (!ImGui::Begin("Toolbar", nullptr, flags)) {
    ImGui::End();
    return;
  }

  const ImVec4 activeColor = ImVec4(0.25f, 0.55f, 0.95f, 1.0f);
  auto toolButton = [&](const char* label, Tool tool) {
    const bool active = editor.currentTool == tool;
    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, activeColor);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
    }
    if (ImGui::Button(label)) {
      editor.currentTool = tool;
    }
    if (active) {
      ImGui::PopStyleColor(3);
    }
  };

  toolButton("Paint", Tool::Paint);
  ImGui::SameLine();
  toolButton("Erase", Tool::Erase);
  ImGui::SameLine();
  toolButton("Rect", Tool::Rect);
  ImGui::SameLine();
  toolButton("Line", Tool::Line);
  ImGui::SameLine();
  toolButton("Stamp", Tool::Stamp);
  ImGui::SameLine();
  toolButton("Fill", Tool::Fill);
  ImGui::SameLine();
  toolButton("Select", Tool::Select);
  ImGui::SameLine();
  toolButton("Move", Tool::Move);
  ImGui::SameLine();
  toolButton("Pan", Tool::Pan);

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

  ImGui::SameLine();
  if (ImGui::Button("?")) {
  }
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted("Hotkeys");
    ImGui::Separator();
    ImGui::TextUnformatted("Q: Paint");
    ImGui::TextUnformatted("W: Rect");
    ImGui::TextUnformatted("E: Fill");
    ImGui::TextUnformatted("R: Erase");
    ImGui::TextUnformatted("I: Pick");
    ImGui::TextUnformatted("[ / ]: Brush Size");
    ImGui::TextUnformatted("Space: Pan (hold)");
    ImGui::TextUnformatted("Ctrl+S: Save");
    ImGui::TextUnformatted("Ctrl+O: Open");
    ImGui::TextUnformatted("Ctrl+Shift+S: Save As");
    ImGui::EndTooltip();
  }

  ImGui::SameLine();
  ImGui::Separator();
  ImGui::SameLine();
  ImGui::TextUnformatted("Brush");
  ImGui::SameLine();
  const int brushSizes[] = {1, 2, 4, 8};
  int brushIndex = 0;
  for (int i = 0; i < 4; ++i) {
    if (editor.brushSize == brushSizes[i]) {
      brushIndex = i;
      break;
    }
  }
  if (ImGui::Combo("##brush_size", &brushIndex, "1\02\04\08\0")) {
    editor.brushSize = brushSizes[brushIndex];
  }

  ImGui::End();
}

void DrawSceneView(EditorUIState& state, EditorUIOutput& out, Framebuffer& framebuffer, float cameraZoom) {
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoBackground;
  if (!ImGui::Begin("Scene View", nullptr, flags)) {
    state.sceneHovered = false;
    state.sceneRect = {};
    out.sceneHovered = false;
    out.sceneActive = false;
    out.sceneRectMin = {};
    out.sceneRectMax = {};
    ImGui::End();
    return;
  }

  if (ImGui::Button("Focus")) {
    out.requestFocus = true;
  }
  ImGui::SameLine();
  ImGui::Checkbox("Grid", &state.showGrid);
  ImGui::SameLine();
  ImGui::Checkbox("Snap", &state.snapEnabled);
  ImGui::SameLine();
  ImGui::TextUnformatted("|");
  ImGui::SameLine();
  ImGui::TextUnformatted("Zoom");
  ImGui::SameLine();
  float zoomPercent = cameraZoom * 100.0f;
  ImGui::SetNextItemWidth(120.0f);
  if (ImGui::SliderFloat("##scene_zoom", &zoomPercent, 25.0f, 200.0f, "%.0f%%")) {
    out.requestSetZoom = true;
    out.zoomValue = zoomPercent / 100.0f;
  }
  ImGui::Separator();

  ImVec2 sceneSize = ImGui::GetContentRegionAvail();
  if (sceneSize.x < 1.0f) sceneSize.x = 1.0f;
  if (sceneSize.y < 1.0f) sceneSize.y = 1.0f;

  const ImGuiIO& io = ImGui::GetIO();
  const int fbWidth = std::max(1, static_cast<int>(sceneSize.x * io.DisplayFramebufferScale.x));
  const int fbHeight = std::max(1, static_cast<int>(sceneSize.y * io.DisplayFramebufferScale.y));
  framebuffer.Resize(fbWidth, fbHeight);

  ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(framebuffer.ColorTexture()));
#if IMGUI_VERSION_NUM >= 19200
  ImGui::Image(ImTextureRef(texId), sceneSize, ImVec2(0, 1), ImVec2(1, 0));
#else
  ImGui::Image(texId, sceneSize, ImVec2(0, 1), ImVec2(1, 0));
#endif
  ImVec2 rectMin = ImGui::GetItemRectMin();
  ImVec2 rectMax = ImGui::GetItemRectMax();
  const bool sceneHovered = ImGui::IsItemHovered();
  const bool sceneActive = ImGui::IsItemActive();
  out.sceneHovered = sceneHovered;
  out.sceneActive = sceneActive;
  out.sceneRectMin = {rectMin.x, rectMin.y};
  out.sceneRectMax = {rectMax.x, rectMax.y};
  state.sceneHovered = sceneHovered;
  state.sceneRect = {rectMin.x, rectMin.y, rectMax.x - rectMin.x, rectMax.y - rectMin.y};

  ImGui::End();
}

void DrawHierarchy(EditorUIState& state, EditorState& editor) {
  if (!ImGui::Begin("Hierarchy")) {
    ImGui::End();
    return;
  }

  if (ImGui::Button("Add Layer")) {
    Layer layer;
    layer.name = "Layer " + std::to_string(editor.layers.size());
    layer.visible = true;
    layer.locked = false;
    layer.opacity = 1.0f;
    layer.tiles.assign(static_cast<size_t>(editor.tileMap.GetWidth() * editor.tileMap.GetHeight()), 0);
    editor.layers.push_back(std::move(layer));
    editor.activeLayer = static_cast<int>(editor.layers.size()) - 1;
    editor.selectedLayer = editor.activeLayer;
  }
  ImGui::SameLine();
  if (ImGui::Button("Duplicate") && editor.activeLayer >= 0 &&
      editor.activeLayer < static_cast<int>(editor.layers.size())) {
    Layer copy = editor.layers[static_cast<size_t>(editor.activeLayer)];
    copy.name += " Copy";
    editor.layers.insert(editor.layers.begin() + editor.activeLayer + 1, copy);
    editor.activeLayer += 1;
    editor.selectedLayer = editor.activeLayer;
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete") && editor.activeLayer >= 0 &&
      editor.activeLayer < static_cast<int>(editor.layers.size())) {
    editor.selectedLayer = editor.activeLayer;
    state.pendingLayerDeleteIndex = editor.activeLayer;
    state.openLayerDeleteModal = true;
  }

  ImGui::Separator();
  if (ImGui::TreeNodeEx("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (size_t i = 0; i < editor.layers.size(); ++i) {
      const bool selected = editor.selectedLayer == static_cast<int>(i);
      std::string label = editor.layers[i].name;
      if (editor.activeLayer == static_cast<int>(i)) {
        label += " (Active)";
      }
      if (ImGui::Selectable(label.c_str(), selected)) {
        editor.selectedLayer = static_cast<int>(i);
        editor.activeLayer = static_cast<int>(i);
      }
    }
    ImGui::TreePop();
  }

  if (editor.activeLayer >= 0 && editor.activeLayer < static_cast<int>(editor.layers.size())) {
    if (ImGui::Button("Move Up") && editor.activeLayer > 0) {
      const std::size_t idx = static_cast<std::size_t>(editor.activeLayer);
      std::swap(editor.layers[idx], editor.layers[idx - 1]);
      editor.activeLayer -= 1;
      editor.selectedLayer = editor.activeLayer;
    }
    ImGui::SameLine();
    if (ImGui::Button("Move Down") && editor.activeLayer + 1 < static_cast<int>(editor.layers.size())) {
      const std::size_t idx = static_cast<std::size_t>(editor.activeLayer);
      std::swap(editor.layers[idx], editor.layers[idx + 1]);
      editor.activeLayer += 1;
      editor.selectedLayer = editor.activeLayer;
    }
  }

  ImGui::End();
}

void DrawInspector(EditorUIState& state, EditorUIOutput& out, EditorState& editor) {
  if (!ImGui::Begin("Inspector")) {
    ImGui::End();
    return;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
  if (editor.selectedLayer >= 0 && editor.selectedLayer < static_cast<int>(editor.layers.size())) {
    ImGui::TextUnformatted("Layer");
    ImGui::Separator();
    if (BeginInspectorTable()) {
      Layer& layer = editor.layers[static_cast<size_t>(editor.selectedLayer)];
      if (state.lastLayerSelection != editor.selectedLayer) {
        EnsureBuffer(state.layerNameBuffer, sizeof(state.layerNameBuffer), layer.name);
        state.lastLayerSelection = editor.selectedLayer;
      }

      InspectorRowLabel("Name");
      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::InputText("##layer_name", state.layerNameBuffer, sizeof(state.layerNameBuffer))) {
        layer.name = state.layerNameBuffer;
      }
      InspectorRowLabel("Visible");
      ImGui::Checkbox("##layer_visible", &layer.visible);
      InspectorRowLabel("Locked");
      ImGui::Checkbox("##layer_locked", &layer.locked);
      InspectorRowLabel("Opacity");
      ImGui::SetNextItemWidth(-1.0f);
      ImGui::SliderFloat("##layer_opacity", &layer.opacity, 0.0f, 1.0f, "%.2f");
      EndInspectorTable();
    }
  } else {
    if (state.pendingMapWidth <= 0) {
      state.pendingMapWidth = editor.tileMap.GetWidth();
    }
    if (state.pendingMapHeight <= 0) {
      state.pendingMapHeight = editor.tileMap.GetHeight();
    }

    bool resetTilemap = false;
    const bool openTilemap = BeginInspectorSection("Tilemap", true, &resetTilemap);
    if (resetTilemap) {
      state.pendingMapWidth = editor.tileMap.GetWidth();
      state.pendingMapHeight = editor.tileMap.GetHeight();
    }
    if (openTilemap) {
      if (BeginInspectorTable()) {
        InspectorRowLabel("Width");
        IntStepper("map_width", &state.pendingMapWidth, 1, 1, 100000);
        InspectorRowLabel("Height");
        IntStepper("map_height", &state.pendingMapHeight, 1, 1, 100000);

        InspectorRowLabel("Tile Size");
        ImGui::Text("%d", editor.tileMap.GetTileSize());

        InspectorRowLabel("Resize");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Button("Apply", ImVec2(-1.0f, 0.0f))) {
          state.openResizeModal = true;
        }
        EndInspectorTable();
      }
      ImGui::TreePop();
    }

    ImGui::Separator();
    bool resetAtlas = false;
    const bool openAtlas = BeginInspectorSection("Atlas", true, &resetAtlas);
    if (resetAtlas) {
      EnsureBuffer(state.atlasPathBuffer, sizeof(state.atlasPathBuffer), editor.atlas.path);
      editor.atlas.tileW = editor.tileMap.GetTileSize();
      editor.atlas.tileH = editor.tileMap.GetTileSize();
      editor.atlas.cols = std::max(1, editor.atlas.cols);
      editor.atlas.rows = std::max(1, editor.atlas.rows);
    }
    if (openAtlas) {
      if (BeginInspectorTable()) {
        if (state.atlasPathBuffer[0] == '\0') {
          EnsureBuffer(state.atlasPathBuffer, sizeof(state.atlasPathBuffer), editor.atlas.path);
        }

        InspectorRowLabel("Path");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##atlas_path", state.atlasPathBuffer, sizeof(state.atlasPathBuffer));
        InspectorRowLabel("Tile W");
        IntStepper("atlas_tile_w", &editor.atlas.tileW, 1, 1, 4096);
        InspectorRowLabel("Tile H");
        IntStepper("atlas_tile_h", &editor.atlas.tileH, 1, 1, 4096);
        InspectorRowLabel("Cols");
        IntStepper("atlas_cols", &editor.atlas.cols, 1, 1, 4096);
        InspectorRowLabel("Rows");
        IntStepper("atlas_rows", &editor.atlas.rows, 1, 1, 4096);

        InspectorRowLabel("Reload");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Button("Reload Atlas", ImVec2(-1.0f, 0.0f))) {
          out.atlasPath = state.atlasPathBuffer;
          out.requestReloadAtlas = true;
        }
        EndInspectorTable();
      }
      ImGui::TreePop();
    }

    ImGui::Separator();
    bool resetView = false;
    const bool openView = BeginInspectorSection("View", true, &resetView);
    if (resetView) {
      editor.sceneBgColor = {0.18f, 0.18f, 0.20f, 1.0f};
      state.showGrid = true;
      state.snapEnabled = false;
    }
    if (openView) {
      if (BeginInspectorTable()) {
        InspectorRowLabel("Background");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::ColorEdit3("##map_bg", &editor.sceneBgColor.r,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
        InspectorRowLabel("Grid");
        ImGui::Checkbox("##view_grid", &state.showGrid);
        InspectorRowLabel("Grid Size");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##grid_size", &state.gridCellSize, 1.0f, 4.0f, "%.1f");
        if (state.gridCellSize < 0.0f) {
          state.gridCellSize = 0.0f;
        }
        InspectorRowLabel("Grid Color");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::ColorEdit3("##grid_color", &state.gridColor.r,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_Float);
        InspectorRowLabel("Grid Alpha");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::SliderFloat("##grid_alpha", &state.gridAlpha, 0.05f, 1.0f, "%.2f");
        InspectorRowLabel("Major Lines");
        IntStepper("grid_major", &state.gridMajorStep, 1, 1, 128);
        InspectorRowLabel("Snap");
        ImGui::Checkbox("##view_snap", &state.snapEnabled);
        EndInspectorTable();
      }
      ImGui::TreePop();
    }
  }

  ImGui::PopStyleVar();
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

  ImGui::Separator();
  ImGui::Text("Autosave");
  ImGui::Checkbox("Enable Autosave", &state.autosaveEnabled);
  ImGui::SliderFloat("Autosave Interval (s)", &state.autosaveInterval, 5.0f, 300.0f, "%.0f");
  if (state.autosaveInterval < 5.0f) {
    state.autosaveInterval = 5.0f;
  }

  ImGui::End();
}

void DrawPreferencesModal(EditorUIState& state) {
  if (state.showPreferences) {
    ImGui::OpenPopup("Preferences");
    state.showPreferences = false;
  }

  if (!ImGui::BeginPopupModal("Preferences", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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

  if (changed) {
    theme.popupBgAlpha = theme.windowBgAlpha;
    state.themeDirty = true;
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Autosave");
  ImGui::Checkbox("Enable Autosave", &state.autosaveEnabled);
  ImGui::SliderFloat("Interval (s)", &state.autosaveInterval, 5.0f, 300.0f, "%.0f");
  if (state.autosaveInterval < 5.0f) {
    state.autosaveInterval = 5.0f;
  }
  if (state.autosavePathBuffer[0] == '\0') {
    EnsureBuffer(state.autosavePathBuffer, sizeof(state.autosavePathBuffer), state.autosavePath);
  }
  if (ImGui::InputText("Path", state.autosavePathBuffer, sizeof(state.autosavePathBuffer))) {
    state.autosavePath = state.autosavePathBuffer;
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Input");
  ImGui::Checkbox("Invert Zoom", &state.invertZoom);
  ImGui::SliderFloat("Pan Speed", &state.panSpeed, 0.2f, 3.0f, "%.2f");
  if (state.panSpeed < 0.1f) {
    state.panSpeed = 0.1f;
  }

  if (ImGui::Button("Close")) {
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();
}

void DrawProjectDirectory(const std::filesystem::path& root,
                          const char* label,
                          bool treatAsMap,
                          bool treatAsTexture,
                          bool treatAsStamp,
                          const char* filter,
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
      DrawProjectDirectory(path, name.c_str(), treatAsMap, treatAsTexture, treatAsStamp, filter,
                           hasUnsavedChanges, state, out);
      continue;
    }

    if (!MatchesProjectFilter(name, filter)) {
      continue;
    }

    const std::string relPath = path.generic_string();
    ImGui::PushID(relPath.c_str());
    if (ImGui::Selectable(name.c_str())) {
      if (treatAsMap) {
        if (hasUnsavedChanges) {
          state.pendingAction = PendingAction::LoadPath;
          state.pendingLoadPath = relPath;
          state.showConfirmOpen = true;
        } else {
          out.requestLoad = true;
          out.loadPath = relPath;
        }
      } else if (treatAsTexture) {
        out.atlasPath = relPath;
        out.requestReloadAtlas = true;
      } else if (treatAsStamp) {
        out.requestLoadStamp = true;
        out.stampPath = relPath;
      }
    }

    if (ImGui::BeginPopupContextItem("project_item_context")) {
      if (treatAsMap && ImGui::MenuItem("Open")) {
        if (hasUnsavedChanges) {
          state.pendingAction = PendingAction::LoadPath;
          state.pendingLoadPath = relPath;
          state.showConfirmOpen = true;
        } else {
          out.requestLoad = true;
          out.loadPath = relPath;
        }
      }
      if (treatAsTexture && ImGui::MenuItem("Set as Atlas")) {
        out.atlasPath = relPath;
        out.requestReloadAtlas = true;
      }
      if (treatAsMap && ImGui::MenuItem("Duplicate")) {
        if (IsPathUnderMaps(path) && std::filesystem::is_regular_file(path)) {
          std::filesystem::path target = MakeMapCopyPath(path);
          std::error_code ec;
          std::filesystem::copy_file(path, target, std::filesystem::copy_options::none, ec);
          if (ec) {
            Log::Warn("Failed to duplicate map.");
          } else {
            Log::Info("Duplicated: " + target.generic_string());
          }
        } else {
          Log::Warn("Failed to duplicate map.");
        }
      }
      if (treatAsMap && ImGui::MenuItem("Delete")) {
        state.pendingDeletePath = relPath;
        state.openDeleteModal = true;
      }
      ImGui::EndPopup();
    }
    ImGui::PopID();
  }

  ImGui::TreePop();
}

void DrawProject(EditorUIState& state, EditorUIOutput& out, const EditorState& editor) {
  if (!ImGui::Begin("Project")) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginPopupContextWindow("ProjectContext", ImGuiPopupFlags_NoOpenOverItems |
                                                      ImGuiPopupFlags_MouseButtonRight)) {
    if (ImGui::MenuItem("New Map...")) {
      std::filesystem::create_directories("assets/maps");
      std::filesystem::path base = std::filesystem::path("assets/maps") / "NewMap.json";
      std::filesystem::path target = MakeUniquePath(base, "");
      if (WriteNewMapFile(target, editor)) {
        Log::Info("Created map: " + target.generic_string());
      } else {
        Log::Error("Failed to create new map.");
      }
    }
    ImGui::EndPopup();
  }

  const int mapCount = CountFilesInDirectory("assets/maps");
  const int textureCount = CountFilesInDirectory("assets/textures");
  ImGui::Text("Textures (%d) / Maps (%d)", textureCount, mapCount);

  ImGui::InputText("Search", state.projectFilter, sizeof(state.projectFilter));
  ImGui::SameLine();
  ImGui::SetNextItemWidth(120.0f);
  ImGui::Combo("##project_filter_mode", &state.projectFilterMode, "All\0Maps\0Textures\0");

  const bool showMaps = state.projectFilterMode == 0 || state.projectFilterMode == 1;
  const bool showTextures = state.projectFilterMode == 0 || state.projectFilterMode == 2;

  if (showMaps) {
    DrawProjectDirectory("assets/maps", "assets/maps", true, false, false, state.projectFilter,
                         editor.hasUnsavedChanges, state, out);
  }
  if (showTextures) {
    DrawProjectDirectory("assets/textures", "assets/textures", false, true, false, state.projectFilter,
                         editor.hasUnsavedChanges, state, out);
  }
  if (state.projectFilterMode == 0 && std::filesystem::exists("assets/stamps")) {
    DrawProjectDirectory("assets/stamps", "assets/stamps", false, false, true, state.projectFilter,
                         editor.hasUnsavedChanges, state, out);
  }
  if (state.projectFilterMode == 0 && std::filesystem::exists("assets/shaders")) {
    DrawProjectDirectory("assets/shaders", "assets/shaders", false, false, false, state.projectFilter,
                         editor.hasUnsavedChanges, state, out);
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
  ImGui::SameLine();
  ImGui::Checkbox("Collapse", &state.consoleCollapse);
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    log.Clear();
    state.consoleSelectedIndex = -1;
    state.consoleSelectedMessage.clear();
  }
  ImGui::InputText("Filter", state.consoleFilter, sizeof(state.consoleFilter));

  struct ConsoleEntry {
    std::string line;
    int count = 1;
  };

  const std::string filterText = state.consoleFilter;
  const auto& lines = log.GetLines();
  std::vector<ConsoleEntry> entries;
  entries.reserve(lines.size());

  for (const std::string& line : lines) {
    const bool isInfo = line.rfind("[Info]", 0) == 0;
    const bool isWarn = line.rfind("[Warn]", 0) == 0;
    const bool isError = line.rfind("[Error]", 0) == 0;

    if ((isInfo && !state.filterInfo) || (isWarn && !state.filterWarn) || (isError && !state.filterError)) {
      continue;
    }
    if (!filterText.empty() && line.find(filterText) == std::string::npos) {
      continue;
    }

    if (state.consoleCollapse) {
      auto it = std::find_if(entries.begin(), entries.end(), [&](const ConsoleEntry& entry) {
        return entry.line == line;
      });
      if (it != entries.end()) {
        it->count += 1;
      } else {
        entries.push_back({line, 1});
      }
    } else {
      entries.push_back({line, 1});
    }
  }

  const float detailsHeight = ImGui::GetTextLineHeightWithSpacing() * 3.5f;
  float listHeight = ImGui::GetContentRegionAvail().y - detailsHeight;
  if (listHeight < ImGui::GetTextLineHeightWithSpacing() * 4.0f) {
    listHeight = ImGui::GetTextLineHeightWithSpacing() * 4.0f;
  }

  if (ImGui::BeginChild("ConsoleList", ImVec2(0.0f, listHeight), true)) {
    for (size_t i = 0; i < entries.size(); ++i) {
      const ConsoleEntry& entry = entries[i];
      const bool isInfo = entry.line.rfind("[Info]", 0) == 0;
      const bool isWarn = entry.line.rfind("[Warn]", 0) == 0;
      const bool isError = entry.line.rfind("[Error]", 0) == 0;

      ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
      if (isWarn) {
        color = ImVec4(0.95f, 0.78f, 0.35f, 1.0f);
      } else if (isError) {
        color = ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
      } else if (isInfo) {
        color = ImVec4(0.65f, 0.85f, 0.95f, 1.0f);
      }

      std::string displayLine = entry.line;
      if (entry.count > 1) {
        displayLine += " (x" + std::to_string(entry.count) + ")";
      }

      ImGui::PushStyleColor(ImGuiCol_Text, color);
      const bool selected = state.consoleSelectedIndex == static_cast<int>(i);
      if (ImGui::Selectable(displayLine.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
        state.consoleSelectedIndex = static_cast<int>(i);
        state.consoleSelectedMessage = entry.line;
      }
      ImGui::PopStyleColor();
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
      ImGui::SetScrollHereY(1.0f);
    }
  }
  ImGui::EndChild();

  ImGui::Separator();
  ImGui::TextUnformatted("Details");
  if (ImGui::BeginChild("ConsoleDetails", ImVec2(0.0f, detailsHeight), true)) {
    if (!state.consoleSelectedMessage.empty()) {
      ImGui::TextWrapped("%s", state.consoleSelectedMessage.c_str());
    } else {
      ImGui::TextDisabled("Select a log entry to view details.");
    }
  }
  ImGui::EndChild();

  ImGui::End();
}

void DrawTilePalette(EditorUIOutput& out, EditorState& editor, const Texture& atlasTexture) {
  if (!ImGui::Begin("Tile Palette")) {
    ImGui::End();
    return;
  }

  if (atlasTexture.IsFallback()) {
    ImGui::TextDisabled("Atlas not loaded.");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Button("Reload Atlas", ImVec2(-1.0f, 0.0f))) {
      out.atlasPath = editor.atlas.path;
      out.requestReloadAtlas = true;
    }
    ImGui::End();
    return;
  }

  const int cols = std::max(1, editor.atlas.cols);
  const int rows = std::max(1, editor.atlas.rows);
  const int total = cols * rows;
  const float buttonSize = 36.0f;
  int hoveredTile = -1;

  ImGui::BeginChild("TilePaletteGrid", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), true);
  ImTextureID textureId = ToImTextureID(atlasTexture);
#if IMGUI_VERSION_NUM >= 19200
  ImTextureRef textureRef(textureId);
#endif
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      const int tileIndex = row * cols + col + 1;
      if (tileIndex > total) {
        break;
      }
      ImGui::PushID(tileIndex);
      bool clicked = false;
      ImVec2 uv0{};
      ImVec2 uv1{};
      if (ComputeAtlasUV(editor.atlas, tileIndex, uv0, uv1)) {
#if IMGUI_VERSION_NUM >= 19200
        clicked = ImGui::ImageButton("##tile", textureRef, ImVec2(buttonSize, buttonSize), uv0, uv1);
#else
        clicked = ImGui::ImageButton("##tile", textureId, ImVec2(buttonSize, buttonSize), uv0, uv1);
#endif
      } else {
        clicked = ImGui::Button("?", ImVec2(buttonSize, buttonSize));
      }

      if (clicked) {
        editor.currentTileIndex = tileIndex;
      }
      if (ImGui::IsItemHovered()) {
        hoveredTile = tileIndex;
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        drawList->AddRect(min, max, IM_COL32(255, 255, 255, 200), 0.0f, 0, 2.0f);
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          editor.currentTileIndex = tileIndex;
          editor.currentTool = Tool::Paint;
        }
      }

      if (col + 1 < cols) {
        ImGui::SameLine();
      }
      ImGui::PopID();
    }
  }
  ImGui::EndChild();

  if (hoveredTile > 0) {
    ImGui::Text("Hover: %d", hoveredTile);
  } else {
    ImGui::Text("Hover: --");
  }
  ImGui::Text("Current: %d", editor.currentTileIndex);

  ImGui::End();
}

void DrawStatusBar(const EditorUIState& state, const EditorState& editor, float zoom, float fps) {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  const float height = ImGui::GetFrameHeightWithSpacing() + 6.0f;
  ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - height));
  ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoDocking;
  if (!ImGui::Begin("Status Bar", nullptr, flags)) {
    ImGui::End();
    return;
  }

  const char* toolLabel = ToolLabel(editor.currentTool);
  const int tileId = editor.currentTileIndex;
  char hoverBuffer[64];
  if (editor.selection.hasHover) {
    std::snprintf(hoverBuffer, sizeof(hoverBuffer), "(%d, %d)", editor.selection.hoverCell.x,
                  editor.selection.hoverCell.y);
  } else {
    std::snprintf(hoverBuffer, sizeof(hoverBuffer), "(--, --)");
  }

  std::string path = state.currentMapPath;
  if (editor.hasUnsavedChanges) {
    path += " *";
  }

  const char* dirtyLabel = editor.hasUnsavedChanges ? "Dirty*" : "Clean";
  const float zoomPercent = zoom * 100.0f;
  ImGui::Text("Tool: %s | Tile: %d | Hover: %s | Zoom: %.0f%% | %s | FPS: %.1f",
              toolLabel, tileId, hoverBuffer, zoomPercent, dirtyLabel, fps);

  if (state.saveMessageTimer > 0.0f) {
    const float textWidth = ImGui::CalcTextSize("Saved").x;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - textWidth - 10.0f);
    ImGui::TextUnformatted("Saved");
  }

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
  if (state.showSaveAs) {
    if (state.saveAsBuffer[0] == '\0') {
      EnsureBuffer(state.saveAsBuffer, sizeof(state.saveAsBuffer), "assets/maps/untitled.json");
    }
    ImGui::OpenPopup("Save As");
    state.showSaveAs = false;
  }

  if (ImGui::BeginPopupModal("Save As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Path", state.saveAsBuffer, sizeof(state.saveAsBuffer));
    if (ImGui::Button("Save")) {
      std::string path = EnsureJsonExtension(state.saveAsBuffer);
      if (path.empty()) {
        Log::Warn("Save As path is empty.");
      } else if (std::filesystem::exists(path)) {
        state.pendingOverwritePath = path;
        state.showOverwriteModal = true;
      } else {
        out.requestSaveAs = true;
        out.saveAsPath = path;
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawOverwriteModal(EditorUIState& state, EditorUIOutput& out) {
  if (state.showOverwriteModal) {
    ImGui::OpenPopup("Overwrite File");
    state.showOverwriteModal = false;
  }

  if (ImGui::BeginPopupModal("Overwrite File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("Overwrite existing file?\n%s", state.pendingOverwritePath.c_str());
    if (ImGui::Button("Overwrite")) {
      out.requestSaveAs = true;
      out.saveAsPath = state.pendingOverwritePath;
      state.pendingOverwritePath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      state.pendingOverwritePath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawOpenModal(EditorUIState& state, EditorUIOutput& out, const EditorState& editor) {
  if (state.showOpenModal) {
    ImGui::OpenPopup("Open Map");
    state.showOpenModal = false;
  }

  if (ImGui::BeginPopupModal("Open Map", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const auto maps = CollectFiles("assets/maps", [](const std::filesystem::path& path) {
      return EndsWith(path.filename().string(), ".json");
    });
    if (maps.empty()) {
      ImGui::TextDisabled("No maps found in assets/maps/");
    } else {
      for (const auto& path : maps) {
        const std::string relPath = path.generic_string();
        if (ImGui::Selectable(relPath.c_str())) {
          if (editor.hasUnsavedChanges) {
            state.pendingAction = PendingAction::LoadPath;
            state.pendingLoadPath = relPath;
            state.showConfirmOpen = true;
          } else {
            out.requestLoad = true;
            out.loadPath = relPath;
          }
          ImGui::CloseCurrentPopup();
        }
      }
    }
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawRecoverAutosaveModal(EditorUIState& state, EditorUIOutput& out, const EditorState& editor) {
  if (state.showRecoverAutosave) {
    ImGui::OpenPopup("Recover Autosave");
    state.showRecoverAutosave = false;
  }

  if (ImGui::BeginPopupModal("Recover Autosave", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    const auto autosaves = CollectFiles("assets/autosave", [](const std::filesystem::path& path) {
      const std::string name = path.filename().string();
      return EndsWith(name, ".autosave.json") || name == "autosave.json";
    });
    if (autosaves.empty()) {
      ImGui::TextDisabled("No autosave files found.");
    } else {
      for (const auto& path : autosaves) {
        const std::string relPath = path.generic_string();
        const std::string stamp = FormatTimestamp(std::filesystem::last_write_time(path));
        ImGui::PushID(relPath.c_str());
        if (ImGui::Selectable(relPath.c_str())) {
          if (editor.hasUnsavedChanges) {
            state.pendingAction = PendingAction::LoadPath;
            state.pendingLoadPath = relPath;
            state.showConfirmOpen = true;
          } else {
            out.requestLoad = true;
            out.loadPath = relPath;
          }
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", stamp.c_str());
        ImGui::PopID();
      }
    }
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawStampModal(EditorUIState& state, EditorUIOutput& out) {
  if (state.openStampModal) {
    ImGui::OpenPopup("Create Stamp");
    state.openStampModal = false;
  }

  if (ImGui::BeginPopupModal("Create Stamp", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Name", state.stampNameBuffer, sizeof(state.stampNameBuffer));
    if (ImGui::Button("Create")) {
      out.requestCreateStamp = true;
      out.stampName = state.stampNameBuffer;
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
  if (state.showAbout) {
    ImGui::OpenPopup("About");
    state.showAbout = false;
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
  if (state.showConfirmOpen || state.showConfirmQuit) {
    ImGui::OpenPopup("Unsaved Changes");
    state.showConfirmOpen = false;
    state.showConfirmQuit = false;
  }

  if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (state.pendingAction == PendingAction::Quit) {
      ImGui::TextWrapped("You have unsaved changes. Save before quitting?");
    } else {
      ImGui::TextWrapped("You have unsaved changes. Save before opening?");
    }
    if (ImGui::Button("Save")) {
      out.confirmSave = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save")) {
      out.confirmDiscard = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      state.pendingAction = PendingAction::None;
      state.pendingLoadPath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawDeleteModal(EditorUIState& state) {
  if (state.openDeleteModal) {
    ImGui::OpenPopup("Delete Asset");
    state.openDeleteModal = false;
  }

  if (ImGui::BeginPopupModal("Delete Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("Delete asset?\n%s", state.pendingDeletePath.c_str());
    if (ImGui::Button("Delete")) {
      std::filesystem::path target = state.pendingDeletePath;
      if (IsPathUnderMaps(target) && std::filesystem::is_regular_file(target)) {
        std::error_code ec;
        std::filesystem::remove(target, ec);
        if (ec) {
          Log::Error("Failed to delete asset.");
        } else {
          Log::Info("Deleted: " + target.generic_string());
        }
      } else {
        Log::Warn("Refused to delete map outside assets/maps/.");
      }
      state.pendingDeletePath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      state.pendingDeletePath.clear();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawLayerDeleteModal(EditorUIState& state, EditorState& editor) {
  if (state.openLayerDeleteModal) {
    ImGui::OpenPopup("Delete Layer");
    state.openLayerDeleteModal = false;
  }

  if (ImGui::BeginPopupModal("Delete Layer", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Delete selected layer?");
    if (ImGui::Button("Delete")) {
      const int index = state.pendingLayerDeleteIndex;
      if (index >= 0 && index < static_cast<int>(editor.layers.size())) {
        if (editor.layers.size() > 1) {
          editor.layers.erase(editor.layers.begin() + index);
          if (editor.activeLayer >= static_cast<int>(editor.layers.size())) {
            editor.activeLayer = static_cast<int>(editor.layers.size()) - 1;
          }
          editor.selectedLayer = editor.activeLayer;
        } else {
          std::fill(editor.layers[0].tiles.begin(), editor.layers[0].tiles.end(), 0);
        }
      }
      state.pendingLayerDeleteIndex = -1;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      state.pendingLayerDeleteIndex = -1;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void BuildDefaultDockLayout(ImGuiViewport* viewport, ImGuiDockNodeFlags dockFlags) {
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

void BuildDockSpace(EditorUIState& state) {
#ifdef IMGUI_HAS_DOCK
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGui::DockSpaceOverViewport(0, viewport, dockFlags);

  const char* iniPath = ImGui::GetIO().IniFilename;
  const bool hasIni = iniPath && std::filesystem::exists(iniPath);
  if (state.requestResetLayout || (!state.dockInitialized && !hasIni)) {
    BuildDefaultDockLayout(viewport, dockFlags);
    state.requestResetLayout = false;
  }

  if (!state.dockInitialized) {
    state.dockInitialized = true;
  }
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
  int autosaveEnabled = state.autosaveEnabled ? 1 : 0;
  if (ParseIntAfterKey(text, "autosaveEnabled", autosaveEnabled)) {
    state.autosaveEnabled = autosaveEnabled != 0;
  }
  ParseFloatAfterKey(text, "autosaveInterval", state.autosaveInterval);
  ParseStringAfterKey(text, "autosavePath", state.autosavePath);
  ParseFloatAfterKey(text, "gridCellSize", state.gridCellSize);
  ParseIntAfterKey(text, "gridMajorStep", state.gridMajorStep);
  ParseFloatAfterKey(text, "gridColorR", state.gridColor.r);
  ParseFloatAfterKey(text, "gridColorG", state.gridColor.g);
  ParseFloatAfterKey(text, "gridColorB", state.gridColor.b);
  ParseFloatAfterKey(text, "gridAlpha", state.gridAlpha);
  int invertZoom = state.invertZoom ? 1 : 0;
  if (ParseIntAfterKey(text, "invertZoom", invertZoom)) {
    state.invertZoom = invertZoom != 0;
  }
  ParseFloatAfterKey(text, "panSpeed", state.panSpeed);

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
  if (state.autosavePath.empty()) {
    state.autosavePath = "assets/autosave/autosave.json";
  }
  if (state.gridMajorStep < 1) {
    state.gridMajorStep = 1;
  }
  if (state.gridAlpha < 0.05f) {
    state.gridAlpha = 0.05f;
  }
  if (state.gridAlpha > 1.0f) {
    state.gridAlpha = 1.0f;
  }
  if (state.panSpeed <= 0.0f) {
    state.panSpeed = 1.0f;
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
                            Framebuffer& sceneFramebuffer,
                            float cameraZoom,
                            float fps) {
  EditorUIOutput out{};

  if (state.themeDirty) {
    ApplyTheme(state.theme);
    state.themeDirty = false;
  }

  BuildDockSpace(state);
  DrawMenuBar(state, out, editor);
  DrawToolbar(state, out, editor, atlasTexture);
  if (state.showScene) {
    DrawSceneView(state, out, sceneFramebuffer, cameraZoom);
  } else {
    state.sceneHovered = false;
    state.sceneRect = {};
    out.sceneHovered = false;
    out.sceneActive = false;
    out.sceneRectMin = {};
    out.sceneRectMax = {};
  }

  if (state.showHierarchy) {
    DrawHierarchy(state, editor);
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
    DrawTilePalette(out, editor, atlasTexture);
  }

  DrawStatusBar(state, editor, cameraZoom, fps);

  DrawSaveAsModal(state, out);
  DrawOverwriteModal(state, out);
  DrawOpenModal(state, out, editor);
  DrawRecoverAutosaveModal(state, out, editor);
  DrawStampModal(state, out);
  DrawAboutModal(state);
  DrawPreferencesModal(state);
  DrawResizeModal(state, out);
  DrawUnsavedModal(state, out);
  DrawDeleteModal(state);
  DrawLayerDeleteModal(state, editor);

  return out;
}

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
                      float viewTop) {
  if (state.sceneRect.width <= 1.0f || state.sceneRect.height <= 1.0f) {
    return;
  }

  const ImVec2 clipMin(state.sceneRect.x, state.sceneRect.y);
  const ImVec2 clipMax(state.sceneRect.x + state.sceneRect.width, state.sceneRect.y + state.sceneRect.height);
  auto ClampToRect = [](float value, float minVal, float maxVal) {
    return std::max(minVal, std::min(value, maxVal));
  };

  const char* toolLabel = ToolLabel(editor.currentTool);
  const char* gridLabel = state.showGrid ? "On" : "Off";
  char zoomBuffer[32];
  std::snprintf(zoomBuffer, sizeof(zoomBuffer), "%.0f%%", zoom * 100.0f);
  char hoverBuffer[32];
  if (editor.selection.hasHover) {
    std::snprintf(hoverBuffer, sizeof(hoverBuffer), "(%d, %d)", editor.selection.hoverCell.x,
                  editor.selection.hoverCell.y);
  } else {
    std::snprintf(hoverBuffer, sizeof(hoverBuffer), "(--, --)");
  }
  char worldBuffer[48];
  if (editor.selection.hasHover) {
    std::snprintf(worldBuffer, sizeof(worldBuffer), "(%.1f, %.1f)", editor.mouseWorld.x, editor.mouseWorld.y);
  } else {
    std::snprintf(worldBuffer, sizeof(worldBuffer), "(--, --)");
  }
  char camBuffer[48];
  std::snprintf(camBuffer, sizeof(camBuffer), "(%.1f, %.1f)", cameraPos.x, cameraPos.y);

  std::string line1 = std::string("Tool: ") + toolLabel;
  std::string line2 = std::string("Grid: ") + gridLabel;
  std::string line3 = std::string("Zoom: ") + zoomBuffer;
  std::string line4 = std::string("Hover: ") + hoverBuffer;
  std::string line5 = std::string("World: ") + worldBuffer;
  std::string line6 = std::string("Camera: ") + camBuffer;

  const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
  const ImVec2 size1 = ImGui::CalcTextSize(line1.c_str());
  const ImVec2 size2 = ImGui::CalcTextSize(line2.c_str());
  const ImVec2 size3 = ImGui::CalcTextSize(line3.c_str());
  const ImVec2 size4 = ImGui::CalcTextSize(line4.c_str());
  const ImVec2 size5 = ImGui::CalcTextSize(line5.c_str());
  const ImVec2 size6 = ImGui::CalcTextSize(line6.c_str());
  float textWidth = size1.x;
  textWidth = std::max(textWidth, size2.x);
  textWidth = std::max(textWidth, size3.x);
  textWidth = std::max(textWidth, size4.x);
  textWidth = std::max(textWidth, size5.x);
  textWidth = std::max(textWidth, size6.x);

  const float padding = 6.0f;
  const float previewSize = 24.0f;
  const float previewPadding = 6.0f;
  const float panelWidth = textWidth + padding * 2.0f + previewSize + previewPadding;
  const float panelHeight = padding * 2.0f + lineHeight * 6.0f;

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  ImVec2 panelPos(state.sceneRect.x + 8.0f, state.sceneRect.y + 8.0f);
  panelPos.x = ClampToRect(panelPos.x, clipMin.x, std::max(clipMin.x, clipMax.x - panelWidth));
  panelPos.y = ClampToRect(panelPos.y, clipMin.y, std::max(clipMin.y, clipMax.y - panelHeight));
  const ImVec2 panelMax(panelPos.x + panelWidth, panelPos.y + panelHeight);

  drawList->PushClipRect(clipMin, clipMax, true);
  drawList->AddRectFilled(panelPos, panelMax, IM_COL32(0, 0, 0, 150), 4.0f);

  ImVec2 textPos(panelPos.x + padding, panelPos.y + padding);
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 230), line1.c_str());
  textPos.y += lineHeight;
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 220), line2.c_str());
  textPos.y += lineHeight;
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 220), line3.c_str());
  textPos.y += lineHeight;
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 220), line4.c_str());
  textPos.y += lineHeight;
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 220), line5.c_str());
  textPos.y += lineHeight;
  drawList->AddText(textPos, IM_COL32(255, 255, 255, 220), line6.c_str());

  const ImVec2 previewMin(panelPos.x + padding + textWidth + previewPadding, panelPos.y + padding);
  const ImVec2 previewMax(previewMin.x + previewSize, previewMin.y + previewSize);

  if (editor.currentTileIndex > 0) {
    if (!atlasTexture.IsFallback()) {
      ImVec2 uv0{};
      ImVec2 uv1{};
      if (ComputeAtlasUV(editor.atlas, editor.currentTileIndex, uv0, uv1)) {
        ImTextureID textureId = ToImTextureID(atlasTexture);
        drawList->AddImage(textureId, previewMin, previewMax, uv0, uv1);
      } else {
        ImVec4 color = TileFallbackColor(editor.currentTileIndex);
        drawList->AddRectFilled(previewMin, previewMax, ImGui::ColorConvertFloat4ToU32(color));
      }
    } else {
      ImVec4 color = TileFallbackColor(editor.currentTileIndex);
      drawList->AddRectFilled(previewMin, previewMax, ImGui::ColorConvertFloat4ToU32(color));
    }
  }
  drawList->AddRect(previewMin, previewMax, IM_COL32(255, 255, 255, 120));

  if (mapWorldWidth > 0.0f && mapWorldHeight > 0.0f) {
    const float miniWidth = 160.0f;
    const float miniHeight = 120.0f;
    const float margin = 8.0f;
    ImVec2 miniPos(state.sceneRect.x + state.sceneRect.width - miniWidth - margin, state.sceneRect.y + margin);
    miniPos.x = ClampToRect(miniPos.x, clipMin.x, std::max(clipMin.x, clipMax.x - miniWidth));
    miniPos.y = ClampToRect(miniPos.y, clipMin.y, std::max(clipMin.y, clipMax.y - miniHeight));
    const ImVec2 miniMax(miniPos.x + miniWidth, miniPos.y + miniHeight);
    drawList->AddRectFilled(miniPos, miniMax, IM_COL32(0, 0, 0, 140), 4.0f);
    drawList->AddRect(miniPos, miniMax, IM_COL32(255, 255, 255, 80), 4.0f);

    const float scale = std::min(miniWidth / mapWorldWidth, miniHeight / mapWorldHeight);
    const float mapDrawW = mapWorldWidth * scale;
    const float mapDrawH = mapWorldHeight * scale;
    const float mapMinX = miniPos.x + (miniWidth - mapDrawW) * 0.5f;
    const float mapMinY = miniPos.y + (miniHeight - mapDrawH) * 0.5f;
    const float mapMaxX = mapMinX + mapDrawW;
    const float mapMaxY = mapMinY + mapDrawH;

    drawList->AddRect(ImVec2(mapMinX, mapMinY), ImVec2(mapMaxX, mapMaxY), IM_COL32(200, 200, 200, 160));

    float viewX0 = mapMinX + viewLeft * scale;
    float viewX1 = mapMinX + viewRight * scale;
    float viewY0 = mapMaxY - viewTop * scale;
    float viewY1 = mapMaxY - viewBottom * scale;
    viewX0 = ClampToRect(viewX0, mapMinX, mapMaxX);
    viewX1 = ClampToRect(viewX1, mapMinX, mapMaxX);
    viewY0 = ClampToRect(viewY0, mapMinY, mapMaxY);
    viewY1 = ClampToRect(viewY1, mapMinY, mapMaxY);
    drawList->AddRect(ImVec2(viewX0, viewY0), ImVec2(viewX1, viewY1), IM_COL32(100, 200, 255, 200));
  }
  drawList->PopClipRect();
}

} // namespace te::ui
