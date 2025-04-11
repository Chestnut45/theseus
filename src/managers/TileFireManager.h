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

    void AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan = -1.0f, float p_cooldown = -1.0f, bool p_reset_burning_lifespan = true);
    void SetPropagationActiveness(bool p_propagation);

private:

    // FireTile Struct
    struct FireTile
    {
        enum BurnState {
            BURNING = 0,
            BURNT,
            UNBURNT,
            DEFAULT_BURNSTATE   // NOT to be used
        };

        enum Direction
        {
            WEST = 0,
            EAST,
            NORTH,
            SOUTH,
            NORTHWEST,
            SOUTHWEST,
            NORTHEAST,
            SOUTHEAST,
        };

        glm::ivec2 m_vTilePos = glm::ivec2(0, 0);
        std::array<glm::ivec2, 8> m_aNeighbourPos;
        std::array<bool, 8> m_aInBounds;                            // Checks if neighbours are within bounds
        std::array<bool, 8> m_aNotWalls;                            // Checks if neighbours are not walls
        std::array<float, 8> m_aNeighbourWeights;                   // Each weight is added by 1 and multiplied by the spread chance
        std::array<glm::ivec2, 4> m_aCardinalNeighbourPairIndices;  // For ordinal propagation optimisation

        wolf::GameObject* m_pFireObj = nullptr;
        wolf::GameObject* m_pBurntTileObj = nullptr;
        BurnState m_currentBurnState = BurnState::UNBURNT;
        float m_fLifespan = 0.0f;
        float m_fBurntCooldown = 0.0f;
        
        float m_fSpreadDelay = 0.2f;    // Delay between propagation attempts
        float m_fSpreadDelayTimer = 0.0f;    // Delay between propagation attempts
        float m_fCardinalSpreadChance = 0.014f;  // Chance of spreading fire to a cardinal neighbour tile
        float m_fOrdinalSpreadChance =  0.01f;  // Chance of spreading fire to an ordinal neighbour tile
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

    bool m_bIsPropagationEnabled = true;
    float m_fStockLifespan = 10.0f;
    float m_fStockBurntCooldown = 10.0f;
    
    std::map<int, std::vector<FireTile*>> m_mFireColumns;   // Arranges fire tiles in columns
    LabyrinthManager* m_pLBMG = nullptr;
    wolf::Scene* m_pScene = nullptr;
    wolf::GameObject* m_pPlayerObj = nullptr;
    int m_iBurningFireTilesCount = 0;
    static TileFireManager* s_pTFMG;

    TileFireManager(LabyrinthManager* p_lbmg);
    ~TileFireManager();
    static bool IsWallTile(int p_tile_id);

public:
    std::vector<glm::ivec2> GetFireTilePositions(int p_burn_state = 3) const;
    int GetBurningFireTilesCount() const;
};