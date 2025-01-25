//-----------------------------------------------------------------------------
// File: TileFireManager.h
// Original Author: Nguyễn Minh Nhật
// Fire on a tile
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class TileFireManager
{
public:
    static void CreateInstance();
    static void DestroyInstance();
    static TileFireManager* GetInstance();
    
    void Update(float p_delta);

    void AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan = 10.0f);
private:

    // FireTile Struct
    struct FireTile
    {
        glm::ivec2 m_vTilePos = glm::ivec2(0, 0);
        float m_fLifespan = 0.0f;
    };

    
    std::map<int, std::vector<FireTile>> m_mVerticalFireStrips;
    
    static TileFireManager* s_pTFMG;

    TileFireManager();
    ~TileFireManager();

    void RemoveFireTile(FireTile p_fire_tile);
};