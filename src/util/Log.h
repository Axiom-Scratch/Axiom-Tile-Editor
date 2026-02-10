#pragma once

#include <string>
#include <vector>

namespace te {

class Log {
public:
  static void Info(const std::string& message);
  static void Warn(const std::string& message);
  static void Error(const std::string& message);
  static void Clear();
  static const std::vector<std::string>& GetLines();
};

} // namespace te
