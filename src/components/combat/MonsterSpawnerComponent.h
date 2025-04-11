#pragma once

//-----------------------------------------------------------------------------
// File: MonsterSpawnerComponent.h
// Original Author: Nguyễn Minh Nhật
// Spawns monsters in a room
// Note: The owner GameObject does NOT need to be moved for this component to move
//-----------------------------------------------------------------------------


#include <wolf.h>
#include <LabyrinthManager.h>

class MonsterSpawnerComponent : public wolf::BaseComponent
{
public:
    struct MonsterSpawnerData
    {
        glm::ivec2 triggerTilePos = glm::ivec2(0, 0);   // Bottom left tile of trigger
        glm::ivec2 triggerSize = glm::ivec2 (1, 1);     // In terms of tiles
        glm::ivec2 spawnerTilePos = glm::ivec2(0, 0);   // Bottom left tile of spawner
        glm::ivec2 spawnerSize = glm::ivec2(1, 1);      // In terms of tiles
        int gorgonCount = 0;
        int harpyCount = 0;
        int minitaurCount = 0;
    };

    MonsterSpawnerComponent(MonsterSpawnerData p_msd);
    ~MonsterSpawnerComponent();
    
    void Init();
    void Update(float p_delta);

private:
    LabyrinthManager* m_pLBMG = nullptr;
    MonsterSpawnerData m_MSData;
    static wolf::RNG s_RNG;
    std::vector<glm::ivec2> m_vAvailableTiles;
    std::vector<glm::ivec2> m_vOccupiedTiles;

    void SpawnMonsters();
    void QueryOccupiedTiles();
    void QueryAvailableTiles();

    void HandleSpawnerBoundLines();
    void HandleTriggerBoundLines();
};