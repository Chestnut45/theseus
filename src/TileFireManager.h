//-----------------------------------------------------------------------------
// File: TileFireManager.h
// Original Author: Nguyễn Minh Nhật
// Fire on a tile
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>
#include "components/LabyrinthManager.h"

class TileFireManager
{
public:
    static void CreateInstance(LabyrinthManager* p_lbmg);
    static void DestroyInstance();
    static TileFireManager* GetInstance();
    
    void Update(float p_delta);
    void Render();

    void AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan = 10.0f);
private:

    // FireTile Struct
    struct FireTile
    {
        glm::ivec2 m_vTilePos = glm::ivec2(0, 0);
        float m_fLifespan = 0.0f;
        wolf::GameObject* m_pFireObj = nullptr;

        FireTile(LabyrinthManager* p_lbmg, glm::ivec2& p_tile_pos, float p_lifespan);
        ~FireTile();
    };

    
    std::map<int, std::vector<FireTile*>> m_mFireColumns;
    std::vector<int> m_vActiveFireColumnsTracker;
    LabyrinthManager* m_pLBMG = nullptr;
    wolf::Scene* m_pScene = nullptr;
    wolf::GameObject* m_pPlayerObj = nullptr;
    bool m_isPlayerChecked = false;
    
    static TileFireManager* s_pTFMG;

    TileFireManager(LabyrinthManager* p_lbmg);
    ~TileFireManager();

};