//-----------------------------------------------------------------------------
// File: TileFireManager.h
// Original Author: Nguyễn Minh Nhật
// Manages tile-based fire.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>
#include "components/LabyrinthManager.h"

class TileFireManager
{
friend class FireTile;
public:
    static void CreateInstance(LabyrinthManager* p_lbmg);
    static void DestroyInstance();
    static TileFireManager* GetInstance();
    
    void Update(float p_delta);
    void Render();

    void AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan = -1.0f, float p_cooldown = -1.0f);
private:

    // FireTile Struct
    struct FireTile
    {
        enum BurnState {
            BURNING,
            BURNT,
            UNBURNT,
            DEFAULT_BURNSTATE   // NOT to be used
        };

        glm::ivec2 m_vTilePos = glm::ivec2(0, 0);
        glm::ivec4 m_vtilePosLRTB = glm::ivec4(-1, -1, -1, -1);

        wolf::GameObject* m_pFireObj = nullptr;
        wolf::GameObject* m_pBurntTileObj = nullptr;
        BurnState m_currentBurnState = BurnState::UNBURNT;
        float m_fLifespan = 0.0f;
        float m_fBurntCooldown = 0.0f;
        
        float m_fSpreadDelay = 0.2f;    // Delay between propagation attempts
        float m_fSpreadDelayTimer = 0.0f;    // Delay between propagation attempts
        float m_fSpreadChance = 0.01f;  // Chance of spreading fire to a neighbour tile
        float m_fAttractChance = 0.0f; // Chance of making fire from a neighbour tile spread to it

        static wolf::RNG s_rng;

        FireTile(LabyrinthManager* p_lbmg, glm::ivec2& p_tile_pos, float p_lifespan, float p_cooldown);
        ~FireTile();
        
        void Update(float p_delta);
        void Reset(float p_lifespan, float p_cooldown);
        void AttemptPropagation();

        void HandleBurningState(float p_delta);
        void HandleBurntState(float p_delta);
        void HandleUnburntState(float p_delta);

        
    };

    float m_fStockLifespan = 10.0f;
    float m_fStockBurntCooldown = 5.0f;
    
    std::map<int, std::vector<FireTile*>> m_mFireColumns;   // Arranges fire tiles in columns
    std::vector<int> m_vActiveFireColumnsTracker;   // Keeps track of the number of active fire tiles in a column
    LabyrinthManager* m_pLBMG = nullptr;
    wolf::Scene* m_pScene = nullptr;
    wolf::GameObject* m_pPlayerObj = nullptr;
    
    static TileFireManager* s_pTFMG;

    TileFireManager(LabyrinthManager* p_lbmg);
    ~TileFireManager();
    bool IsWallTile(int p_tile_id);
};