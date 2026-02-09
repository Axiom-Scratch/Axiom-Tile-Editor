#pragma once

#include <string>

namespace te {

struct Atlas {
  std::string path;
  int tileW = 0;
  int tileH = 0;
  int cols = 0;
  int rows = 0;
};

} // namespace te
