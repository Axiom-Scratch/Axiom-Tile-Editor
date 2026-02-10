#pragma once

#include "editor/Atlas.h"
#include "util/FileIO.h"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace te::JsonLite {

struct LayerInfo {
  std::string name;
  bool visible = true;
  bool locked = false;
  float opacity = 1.0f;
  std::vector<int> data;
};

inline bool WriteTileMap(const std::string& path, int width, int height, int tileSize,
                         const Atlas& atlas, const std::vector<LayerInfo>& layers) {
  std::ostringstream ss;
  ss << "{\n";
  ss << "  \"version\": 2,\n";
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
  ss << "  \"layers\": [\n";
  for (size_t i = 0; i < layers.size(); ++i) {
    const LayerInfo& layer = layers[i];
    ss << "    {\n";
    ss << "      \"name\": \"" << layer.name << "\",\n";
    ss << "      \"visible\": " << (layer.visible ? 1 : 0) << ",\n";
    ss << "      \"locked\": " << (layer.locked ? 1 : 0) << ",\n";
    ss << "      \"opacity\": " << layer.opacity << ",\n";
    ss << "      \"data\": [";
    for (size_t d = 0; d < layer.data.size(); ++d) {
      ss << layer.data[d];
      if (d + 1 < layer.data.size()) {
        ss << ", ";
      }
    }
    ss << "]\n";
    ss << "    }";
    if (i + 1 < layers.size()) {
      ss << ",";
    }
    ss << "\n";
  }
  ss << "  ],\n";
  if (!layers.empty()) {
    ss << "  \"data\": [";
    for (size_t i = 0; i < layers[0].data.size(); ++i) {
      ss << layers[0].data[i];
      if (i + 1 < layers[0].data.size()) {
        ss << ", ";
      }
    }
    ss << "]\n";
  } else {
    ss << "  \"data\": []\n";
  }
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

inline bool ParseFloatAfterKey(const std::string& text, const std::string& key, float& outValue) {
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
  while (end < text.size() &&
         (std::isdigit(static_cast<unsigned char>(text[end])) || text[end] == '-' || text[end] == '.')) {
    ++end;
  }
  if (end == pos) {
    return false;
  }
  try {
    outValue = std::stof(text.substr(pos, end - pos));
    return true;
  } catch (...) {
    return false;
  }
}

inline bool ParseDataArray(const std::string& text, const std::string& key, std::vector<int>& outData) {
  size_t dataPos = text.find("\"" + key + "\"");
  if (dataPos == std::string::npos) {
    return false;
  }

  dataPos = text.find('[', dataPos);
  size_t endPos = text.find(']', dataPos);
  if (dataPos == std::string::npos || endPos == std::string::npos || endPos <= dataPos) {
    return false;
  }

  std::string dataText = text.substr(dataPos + 1, endPos - dataPos - 1);
  std::stringstream ss(dataText);
  outData.clear();
  while (ss.good()) {
    int value = 0;
    ss >> value;
    if (ss.fail()) {
      break;
    }
    outData.push_back(value);
    if (ss.peek() == ',') {
      ss.ignore();
    }
  }
  return true;
}

inline std::vector<std::string> ExtractLayerObjects(const std::string& text) {
  std::vector<std::string> objects;
  size_t layersPos = text.find("\"layers\"");
  if (layersPos == std::string::npos) {
    return objects;
  }
  size_t arrayStart = text.find('[', layersPos);
  if (arrayStart == std::string::npos) {
    return objects;
  }
  size_t arrayEnd = text.find(']', arrayStart);
  if (arrayEnd == std::string::npos) {
    return objects;
  }

  int depth = 0;
  size_t objStart = std::string::npos;
  for (size_t i = arrayStart; i < arrayEnd; ++i) {
    char c = text[i];
    if (c == '{') {
      if (depth == 0) {
        objStart = i;
      }
      ++depth;
    } else if (c == '}') {
      --depth;
      if (depth == 0 && objStart != std::string::npos) {
        objects.push_back(text.substr(objStart, i - objStart + 1));
        objStart = std::string::npos;
      }
    }
  }
  return objects;
}

inline bool ReadTileMap(const std::string& path, int& width, int& height, int& tileSize, Atlas& atlas,
                        const Atlas& defaultAtlas, std::vector<LayerInfo>& layers,
                        std::string* errorOut = nullptr) {
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

  layers.clear();
  std::vector<std::string> layerObjects = ExtractLayerObjects(text);
  for (size_t i = 0; i < layerObjects.size(); ++i) {
    const std::string& layerText = layerObjects[i];
    LayerInfo layer;
    std::string name;
    if (ParseStringAfterKey(layerText, "name", name)) {
      layer.name = name;
    } else {
      layer.name = "Layer " + std::to_string(i);
    }
    int visible = layer.visible ? 1 : 0;
    int locked = layer.locked ? 1 : 0;
    if (ParseIntAfterKey(layerText, "visible", visible)) {
      layer.visible = visible != 0;
    }
    if (ParseIntAfterKey(layerText, "locked", locked)) {
      layer.locked = locked != 0;
    }
    ParseFloatAfterKey(layerText, "opacity", layer.opacity);

    std::vector<int> data;
    if (!ParseDataArray(layerText, "data", data)) {
      continue;
    }
    if (static_cast<int>(data.size()) != width * height) {
      if (errorOut) {
        *errorOut = "Layer data size does not match width/height.";
      }
      return false;
    }
    layer.data = std::move(data);
    layers.push_back(std::move(layer));
  }

  if (layers.empty()) {
    std::vector<int> data;
    if (!ParseDataArray(text, "data", data)) {
      if (errorOut) {
        *errorOut = "Missing data array.";
      }
      return false;
    }
    if (static_cast<int>(data.size()) != width * height) {
      if (errorOut) {
        *errorOut = "Data size does not match width/height.";
      }
      return false;
    }
    LayerInfo layer;
    layer.name = "Layer 0";
    layer.visible = true;
    layer.locked = false;
    layer.opacity = 1.0f;
    layer.data = std::move(data);
    layers.push_back(std::move(layer));
  }

  return true;
}

} // namespace te::JsonLite
