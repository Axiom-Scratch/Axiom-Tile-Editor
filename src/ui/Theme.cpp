#include "ui/Theme.h"

#include <algorithm>

namespace te::ui {

namespace {

float Clamp01(float v) {
  return std::max(0.0f, std::min(1.0f, v));
}

ImVec4 LiftColor(const ImVec4& color, float lift, float alpha) {
  return ImVec4(Clamp01(color.x + lift), Clamp01(color.y + lift), Clamp01(color.z + lift), alpha);
}

} // namespace

void ApplyTheme(const ThemeSettings& s) {
  if (s.preset == ThemePreset::Light) {
    ImGui::StyleColorsLight();
  } else {
    ImGui::StyleColorsDark();
  }

  ImGuiStyle& style = ImGui::GetStyle();
  style.Alpha = s.globalAlpha;
  style.WindowRounding = s.rounding;
  style.FrameRounding = s.rounding;
  style.PopupRounding = s.rounding;
  style.TabRounding = s.rounding;

  ImVec4* colors = style.Colors;

  if (s.preset == ThemePreset::UnityDark) {
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.20f, s.windowBgAlpha);
    colors[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.18f, 0.20f, s.windowBgAlpha);
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, s.windowBgAlpha);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.21f, s.windowBgAlpha);
    colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.25f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.32f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.34f, s.frameBgAlpha);
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.23f, s.windowBgAlpha);
    colors[ImGuiCol_TabActive] = ImVec4(0.26f, 0.26f, 0.30f, s.windowBgAlpha);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.18f, s.popupBgAlpha);
  } else if (s.preset == ThemePreset::Dark) {
    colors[ImGuiCol_WindowBg] = LiftColor(colors[ImGuiCol_WindowBg], 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_ChildBg] = LiftColor(colors[ImGuiCol_ChildBg], 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_TitleBg] = LiftColor(colors[ImGuiCol_TitleBg], 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_TitleBgActive] = LiftColor(colors[ImGuiCol_TitleBgActive], 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_FrameBg] = LiftColor(colors[ImGuiCol_FrameBg], 0.05f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgHovered] = LiftColor(colors[ImGuiCol_FrameBgHovered], 0.05f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgActive] = LiftColor(colors[ImGuiCol_FrameBgActive], 0.05f, s.frameBgAlpha);
    colors[ImGuiCol_Tab] = LiftColor(colors[ImGuiCol_Tab], 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_TabActive] = LiftColor(colors[ImGuiCol_TabActive], 0.05f, s.windowBgAlpha);
    colors[ImGuiCol_PopupBg] = LiftColor(colors[ImGuiCol_PopupBg], 0.03f, s.popupBgAlpha);
  } else {
    colors[ImGuiCol_WindowBg].w = s.windowBgAlpha;
    colors[ImGuiCol_ChildBg].w = s.windowBgAlpha;
    colors[ImGuiCol_TitleBg].w = s.windowBgAlpha;
    colors[ImGuiCol_TitleBgActive].w = s.windowBgAlpha;
    colors[ImGuiCol_FrameBg].w = s.frameBgAlpha;
    colors[ImGuiCol_FrameBgHovered].w = s.frameBgAlpha;
    colors[ImGuiCol_FrameBgActive].w = s.frameBgAlpha;
    colors[ImGuiCol_Tab].w = s.windowBgAlpha;
    colors[ImGuiCol_TabActive].w = s.windowBgAlpha;
    colors[ImGuiCol_PopupBg].w = s.popupBgAlpha;
  }

  colors[ImGuiCol_DockingEmptyBg].w = std::max(0.1f, s.windowBgAlpha * 0.35f);
  colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
}

} // namespace te::ui
