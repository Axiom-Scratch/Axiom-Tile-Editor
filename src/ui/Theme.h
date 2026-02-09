#pragma once

#include <imgui.h>

namespace te::ui {

enum class ThemePreset {
  Dark,
  TrueDark,
  UnityDark,
  Light
};

struct ThemeSettings {
  ThemePreset preset = ThemePreset::TrueDark;
  float globalAlpha = 1.0f;
  float windowBgAlpha = 0.98f;
  float frameBgAlpha = 0.95f;
  float popupBgAlpha = 0.98f;
  float rounding = 4.0f;
  bool boostContrast = false;
};

void ApplyTheme(const ThemeSettings& s);

} // namespace te::ui
