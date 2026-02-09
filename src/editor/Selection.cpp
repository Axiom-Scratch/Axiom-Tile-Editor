#include "editor/Selection.h"

#include <algorithm>

namespace te {

namespace {

int IndexFor(int x, int y, int width) {
  return y * width + x;
}

bool InBounds(int x, int y, int width, int height) {
  return x >= 0 && y >= 0 && x < width && y < height;
}

} // namespace

void Selection::Resize(int w, int h) {
  width = w;
  height = h;
  mask.assign(static_cast<size_t>(width * height), 0U);
  indices.clear();
  isSelecting = false;
}

void Selection::Clear() {
  std::fill(mask.begin(), mask.end(), 0U);
  indices.clear();
}

bool Selection::IsSelected(int index) const {
  if (index < 0 || index >= static_cast<int>(mask.size())) {
    return false;
  }
  return mask[static_cast<size_t>(index)] != 0U;
}

void Selection::SetSelected(int index, bool selected) {
  if (index < 0 || index >= static_cast<int>(mask.size())) {
    return;
  }
  unsigned char value = selected ? 1U : 0U;
  if (mask[static_cast<size_t>(index)] == value) {
    return;
  }
  mask[static_cast<size_t>(index)] = value;
  if (selected) {
    indices.push_back(index);
  } else {
    auto it = std::find(indices.begin(), indices.end(), index);
    if (it != indices.end()) {
      *it = indices.back();
      indices.pop_back();
    }
  }
}

void Selection::ApplyRect(const Vec2i& a, const Vec2i& b, SelectionMode mode) {
  if (width <= 0 || height <= 0) {
    return;
  }

  int minX = std::min(a.x, b.x);
  int maxX = std::max(a.x, b.x);
  int minY = std::min(a.y, b.y);
  int maxY = std::max(a.y, b.y);

  minX = std::max(0, minX);
  minY = std::max(0, minY);
  maxX = std::min(width - 1, maxX);
  maxY = std::min(height - 1, maxY);

  if (mode == SelectionMode::Replace) {
    Clear();
  }

  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      int idx = IndexFor(x, y, width);
      if (mode == SelectionMode::Toggle) {
        SetSelected(idx, !IsSelected(idx));
      } else {
        SetSelected(idx, true);
      }
    }
  }
}

void Selection::BeginRect(const Vec2i& cell) {
  if (!InBounds(cell.x, cell.y, width, height)) {
    isSelecting = false;
    return;
  }
  isSelecting = true;
  selectStart = cell;
  selectEnd = cell;
}

void Selection::UpdateRect(const Vec2i& cell) {
  if (!isSelecting) {
    return;
  }
  selectEnd = cell;
}

void Selection::EndRect(SelectionMode mode) {
  if (!isSelecting) {
    return;
  }
  ApplyRect(selectStart, selectEnd, mode);
  isSelecting = false;
}

} // namespace te
