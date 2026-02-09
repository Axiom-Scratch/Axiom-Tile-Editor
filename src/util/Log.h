#pragma once

#include <string>

namespace te {

class Log {
public:
  static void Info(const std::string& message);
  static void Warn(const std::string& message);
  static void Error(const std::string& message);
};

} // namespace te
