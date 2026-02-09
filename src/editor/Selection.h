#pragma once

#include "app/Config.h"

namespace te {

struct Selection {
  bool hasHover = false;
  Vec2i hoverCell{};
};

} // namespace te
