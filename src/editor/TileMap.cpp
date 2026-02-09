#include "editor/TileMap.h"

namespace te {

TileMap::TileMap() {
  Resize(0, 0, 0);
}

TileMap::TileMap(int width, int height, int tileSize) {
  Resize(width, height, tileSize);
}

void TileMap::Resize(int width, int height, int tileSize) {
  m_width = width;
  m_height = height;
  m_tileSize = tileSize;
  m_tiles.assign(static_cast<size_t>(m_width * m_height), 0);
}

bool TileMap::IsInBounds(int x, int y) const {
  return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

int TileMap::Index(int x, int y) const {
  return y * m_width + x;
}

int TileMap::GetTile(int x, int y) const {
  if (!IsInBounds(x, y)) {
    return 0;
  }
  return m_tiles[static_cast<size_t>(Index(x, y))];
}

void TileMap::SetTile(int x, int y, int id) {
  if (!IsInBounds(x, y)) {
    return;
  }
  m_tiles[static_cast<size_t>(Index(x, y))] = id;
}

void TileMap::SetData(int width, int height, int tileSize, std::vector<int> data) {
  m_width = width;
  m_height = height;
  m_tileSize = tileSize;
  m_tiles = std::move(data);
}

} // namespace te
