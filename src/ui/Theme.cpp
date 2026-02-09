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

ImVec4 BoostColor(const ImVec4& color, float boost) {
  return ImVec4(Clamp01(color.x + boost), Clamp01(color.y + boost), Clamp01(color.z + boost), color.w);
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

  if (s.preset == ThemePreset::TrueDark) {
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.06f, s.windowBgAlpha);
    colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.06f, s.windowBgAlpha);
    colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.08f, s.popupBgAlpha);
    colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.03f, 0.04f, s.windowBgAlpha);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, s.windowBgAlpha);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.20f, s.frameBgAlpha);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.24f, s.frameBgAlpha);
    colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.10f, s.windowBgAlpha);
    colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.16f, 0.20f, s.windowBgAlpha);
    colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.12f, 0.16f, s.windowBgAlpha);
    colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.18f, s.windowBgAlpha);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.26f, s.windowBgAlpha);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.26f, 0.30f, s.windowBgAlpha);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.04f, 0.04f, 0.05f, s.windowBgAlpha);
    colors[ImGuiCol_Text] = ImVec4(0.98f, 0.98f, 0.98f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.58f, 1.0f);
  } else if (s.preset == ThemePreset::UnityDark) {
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

  if (s.preset == ThemePreset::TrueDark) {
    colors[ImGuiCol_DockingEmptyBg].w = s.windowBgAlpha;
  } else {
    colors[ImGuiCol_DockingEmptyBg].w = std::max(0.1f, s.windowBgAlpha * 0.35f);
  }
  if (colors[ImGuiCol_Text].w <= 0.0f) {
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
  }

  if (s.boostContrast) {
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.65f, 0.65f, 0.68f, 1.0f);
    colors[ImGuiCol_Header] = BoostColor(colors[ImGuiCol_Header], 0.05f);
    colors[ImGuiCol_HeaderHovered] = BoostColor(colors[ImGuiCol_HeaderHovered], 0.05f);
    colors[ImGuiCol_HeaderActive] = BoostColor(colors[ImGuiCol_HeaderActive], 0.05f);
    colors[ImGuiCol_TabActive] = BoostColor(colors[ImGuiCol_TabActive], 0.04f);
  }
}

} // namespace te::ui
