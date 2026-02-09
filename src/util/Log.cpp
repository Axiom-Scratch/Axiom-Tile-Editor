#include "util/Log.h"

#include <iostream>

namespace te {

void Log::Info(const std::string& message) {
  std::cout << "[Info] " << message << "\n";
}

void Log::Warn(const std::string& message) {
  std::cout << "[Warn] " << message << "\n";
}

void Log::Error(const std::string& message) {
  std::cerr << "[Error] " << message << "\n";
}

} // namespace te
