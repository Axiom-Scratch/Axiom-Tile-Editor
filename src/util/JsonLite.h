#pragma once

#include "editor/Atlas.h"
#include "util/FileIO.h"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace te::JsonLite {

inline bool WriteTileMap(const std::string& path, int width, int height, int tileSize,
                         const Atlas& atlas, const std::vector<int>& data) {
  std::ostringstream ss;
  ss << "{\n";
  ss << "  \"width\": " << width << ",\n";
  ss << "  \"height\": " << height << ",\n";
  ss << "  \"tileSize\": " << tileSize << ",\n";
  ss << "  \"atlas\": {\n";
  ss << "    \"path\": \"" << atlas.path << "\",\n";
  ss << "    \"tileW\": " << atlas.tileW << ",\n";
  ss << "    \"tileH\": " << atlas.tileH << ",\n";
  ss << "    \"cols\": " << atlas.cols << ",\n";
  ss << "    \"rows\": " << atlas.rows << "\n";
  ss << "  },\n";
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

inline bool ParseStringAfterKey(const std::string& text, const std::string& key, std::string& outValue) {
  const std::string token = "\"" + key + "\"";
  size_t pos = text.find(token);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find(':', pos);
  if (pos == std::string::npos) {
    return false;
  }
  pos = text.find('"', pos);
  if (pos == std::string::npos) {
    return false;
  }
  ++pos;
  std::string value;
  bool escape = false;
  for (; pos < text.size(); ++pos) {
    char c = text[pos];
    if (!escape && c == '"') {
      break;
    }
    if (!escape && c == '\\') {
      escape = true;
      continue;
    }
    escape = false;
    value.push_back(c);
  }
  outValue = value;
  return true;
}

inline bool ReadTileMap(const std::string& path, int& width, int& height, int& tileSize, Atlas& atlas,
                        const Atlas& defaultAtlas, std::vector<int>& data, std::string* errorOut = nullptr) {
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

  atlas = defaultAtlas;
  bool hasAtlas = false;
  std::string atlasPath;
  int atlasTileW = 0;
  int atlasTileH = 0;
  int atlasCols = 0;
  int atlasRows = 0;
  if (ParseStringAfterKey(text, "path", atlasPath)) {
    atlas.path = atlasPath;
    hasAtlas = true;
  }
  if (ParseIntAfterKey(text, "tileW", atlasTileW)) {
    atlas.tileW = atlasTileW;
    hasAtlas = true;
  }
  if (ParseIntAfterKey(text, "tileH", atlasTileH)) {
    atlas.tileH = atlasTileH;
    hasAtlas = true;
  }
  if (ParseIntAfterKey(text, "cols", atlasCols)) {
    atlas.cols = atlasCols;
    hasAtlas = true;
  }
  if (ParseIntAfterKey(text, "rows", atlasRows)) {
    atlas.rows = atlasRows;
    hasAtlas = true;
  }
  if (!hasAtlas) {
    atlas.tileW = tileSize;
    atlas.tileH = tileSize;
  }

  return true;
}

} // namespace te::JsonLite
