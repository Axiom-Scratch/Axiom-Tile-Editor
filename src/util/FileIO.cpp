#include "util/FileIO.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace te::FileIO {

bool ReadTextFile(const std::string& path, std::string& outText) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  outText = ss.str();
  return true;
}

bool WriteTextFile(const std::string& path, const std::string& text) {
  std::filesystem::path fsPath(path);
  if (fsPath.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(fsPath.parent_path(), ec);
  }

  std::ofstream file(path, std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  file << text;
  return true;
}

} // namespace te::FileIO
