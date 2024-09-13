#include "TileMap.h"

TileMap::TileMap(int width, int height)
    :   m_width(width),
        m_height(height),
        m_tileData(width * height, 0)
{
}

TileMap::~TileMap()
{
}

bool TileMap::LoadTileSet(const std::string& filepath)
{
    // TODO: Parse file line by line, validate texture size, stop at 2048.
    return false;
}

int TileMap::GetTile(int x, int y) const
{
    return 0;
}

void TileMap::SetTile(int x, int y, int tileID)
{

}

void TileMap::Clear()
{

}

void TileMap::Resize(int width, int height)
{
    
}