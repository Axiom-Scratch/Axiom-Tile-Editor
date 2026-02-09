#pragma once

#include "util/FileIO.h"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace te::JsonLite {

inline bool WriteTileMap(const std::string& path, int width, int height, int tileSize,
                         const std::vector<int>& data) {
  std::ostringstream ss;
  ss << "{\n";
  ss << "  \"width\": " << width << ",\n";
  ss << "  \"height\": " << height << ",\n";
  ss << "  \"tileSize\": " << tileSize << ",\n";
  ss << "  \"data\": [";
  for (size_t i = 0; i < data.size(); ++i) {
    ss << data[i];
    if (i + 1 < data.size()) {
      ss << ", ";
    }
  }
  ss << "]\n";
  ss << "}\n";

  return FileIO::WriteTextFile(path, ss.str());
}

inline bool ParseIntAfterKey(const std::string& text, const std::string& key, int& outValue) {
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find(':', pos);
  if (pos == std::string::npos) {
    return false;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  size_t end = pos;
  while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) || text[end] == '-')) {
    ++end;
  }
  if (end == pos) {
    return false;
  }
  try {
    outValue = std::stoi(text.substr(pos, end - pos));
    return true;
  } catch (...) {
    return false;
  }
}

inline bool ReadTileMap(const std::string& path, int& width, int& height, int& tileSize,
                        std::vector<int>& data, std::string* errorOut = nullptr) {
  std::string text;
  if (!FileIO::ReadTextFile(path, text)) {
    if (errorOut) {
      *errorOut = "Failed to read file.";
    }
    return false;
  }

  if (!ParseIntAfterKey(text, "width", width) || !ParseIntAfterKey(text, "height", height) ||
      !ParseIntAfterKey(text, "tileSize", tileSize)) {
    if (errorOut) {
      *errorOut = "Missing required fields.";
    }
    return false;
  }

  size_t dataPos = text.find("\"data\"");
  if (dataPos == std::string::npos) {
    if (errorOut) {
      *errorOut = "Missing data array.";
    }
    return false;
  }

  dataPos = text.find('[', dataPos);
  size_t endPos = text.find(']', dataPos);
  if (dataPos == std::string::npos || endPos == std::string::npos || endPos <= dataPos) {
    if (errorOut) {
      *errorOut = "Invalid data array.";
    }
    return false;
  }

  std::string dataText = text.substr(dataPos + 1, endPos - dataPos - 1);
  std::stringstream ss(dataText);
  data.clear();
  while (ss.good()) {
    int value = 0;
    ss >> value;
    if (ss.fail()) {
      break;
    }
    data.push_back(value);
    if (ss.peek() == ',') {
      ss.ignore();
    }
  }

  if (static_cast<int>(data.size()) != width * height) {
    if (errorOut) {
      *errorOut = "Data size does not match width/height.";
    }
    return false;
  }

  return true;
}

} // namespace te::JsonLite
