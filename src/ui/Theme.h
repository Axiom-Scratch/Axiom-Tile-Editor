#pragma once

#include <imgui.h>

namespace te::ui {

enum class ThemePreset {
  Dark,
  UnityDark,
  Light
};

struct ThemeSettings {
  ThemePreset preset = ThemePreset::UnityDark;
  float globalAlpha = 0.95f;
  float windowBgAlpha = 0.75f;
  float frameBgAlpha = 0.85f;
  float popupBgAlpha = 0.9f;
  float rounding = 4.0f;
};

void ApplyTheme(const ThemeSettings& s);

} // namespace te::ui
