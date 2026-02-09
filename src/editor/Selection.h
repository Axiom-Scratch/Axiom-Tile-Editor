#pragma once

#include "app/Config.h"

#include <vector>

namespace te {

enum class SelectionMode {
  Replace,
  Add,
  Toggle
};

struct Selection {
  bool hasHover = false;
  Vec2i hoverCell{};

  int width = 0;
  int height = 0;
  std::vector<unsigned char> mask;
  std::vector<int> indices;

  bool isSelecting = false;
  Vec2i selectStart{};
  Vec2i selectEnd{};

  void Resize(int w, int h);
  void Clear();
  bool IsSelected(int index) const;
  void SetSelected(int index, bool selected);
  void ApplyRect(const Vec2i& a, const Vec2i& b, SelectionMode mode);
  void BeginRect(const Vec2i& cell);
  void UpdateRect(const Vec2i& cell);
  void EndRect(SelectionMode mode);
  bool HasSelection() const { return !indices.empty(); }
};

} // namespace te
