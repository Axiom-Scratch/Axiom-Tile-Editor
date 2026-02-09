#pragma once

#include <vector>

namespace te {

class TileMap {
public:
  TileMap();
  TileMap(int width, int height, int tileSize);

  void Resize(int width, int height, int tileSize);

  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  int GetTileSize() const { return m_tileSize; }

  bool IsInBounds(int x, int y) const;
  int GetTile(int x, int y) const;
  void SetTile(int x, int y, int id);

  int Index(int x, int y) const;

  const std::vector<int>& GetData() const { return m_tiles; }
  void SetData(int width, int height, int tileSize, std::vector<int> data);

private:
  int m_width = 0;
  int m_height = 0;
  int m_tileSize = 0;
  std::vector<int> m_tiles;
};

} // namespace te
