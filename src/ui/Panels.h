#pragma once

#include "app/Config.h"
#include "editor/Tools.h"
#include "render/OrthoCamera.h"
#include "util/Log.h"

namespace te::ui {

struct HoverInfo {
  bool hasHover = false;
  Vec2i cell{};
};

void DrawDockSpace();
void DrawTilePalette(EditorState& state);
void DrawInspector(const EditorState& state, const OrthoCamera& camera, const HoverInfo& hover);
void DrawLog(Log& log);

} // namespace te::ui
