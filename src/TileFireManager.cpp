#include "TileFireManager.h"

void TileFireManager::Update(float p_delta)
{

    for(auto itr = m_mVerticalFireStrips.begin(); itr != m_mVerticalFireStrips.end();)
    {
        auto fireColumn = itr->second;

        // Update each fire tile
        for (auto fireTile = fireColumn.begin(); fireTile != fireColumn.end();)
        {
            fireTile->m_fLifespan -= p_delta;
            if(fireTile->m_fLifespan <= 0.0f)
            {
                fireTile = fireColumn.erase(fireTile);
            }
            else
            {
                fireTile++;
            }
        }
        
        // If no fire tile remains in a column, erase the column
        if(fireColumn.size() == 0)
        {
            m_mVerticalFireStrips.erase(itr->first);
        }
        else
        {
            itr++;
        }
    }
}

void TileFireManager::AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan)
{
    int column = p_tile_pos.x;

    auto itr = m_mVerticalFireStrips.find(column);
    if(itr == m_mVerticalFireStrips.end())
    {
        m_mVerticalFireStrips.insert({column, {}});
    }

    if(m_mVerticalFireStrips.at(column).size() > 0)
    {
        for(auto fireTile : m_mVerticalFireStrips.at(column))
        {
            if(fireTile.m_vTilePos.y == p_tile_pos.y)
            {
                fireTile.m_fLifespan = p_lifespan;
                return;
            }
        }
    }

    m_mVerticalFireStrips.at(column).emplace_back(FireTile());
    int index = m_mVerticalFireStrips.at(column).size() - 1;
    FireTile* fireTile = &m_mVerticalFireStrips.at(column).at(index);

    fireTile->m_fLifespan = p_lifespan;
    fireTile->m_vTilePos = p_tile_pos;
    return;
}