#include "util/Log.h"

#include <iostream>
#include <vector>

namespace te {

namespace {

constexpr size_t kMaxLines = 200;
std::vector<std::string> s_lines;

void PushLine(const std::string& line) {
  s_lines.push_back(line);
  if (s_lines.size() > kMaxLines) {
    s_lines.erase(s_lines.begin());
  }
}

} // namespace

void Log::Info(const std::string& message) {
  const std::string line = "[Info] " + message;
  std::cout << line << "\n";
  PushLine(line);
}

void Log::Warn(const std::string& message) {
  const std::string line = "[Warn] " + message;
  std::cout << line << "\n";
  PushLine(line);
}

void Log::Error(const std::string& message) {
  const std::string line = "[Error] " + message;
  std::cerr << line << "\n";
  PushLine(line);
}

const std::vector<std::string>& Log::GetLines() {
  return s_lines;
}

} // namespace te
