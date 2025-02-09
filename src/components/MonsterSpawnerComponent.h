//-----------------------------------------------------------------------------
// File: MonsterSpawnerComponent.h
// Original Author: Nguyễn Minh Nhật
// Spawns monsters in a room
// Note: Game object must NOT move for the component to work
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include <LabyrinthManager.h>

class MonsterSpawnerComponent : public wolf::BaseComponent
{
public:
    enum MonsterType
    {
        GORGON,
        HARPY,
        MINITAUR,
        NONE
    };

    struct MonsterSpawnerData
    {
        glm::ivec2 spawnerTilePos = glm::ivec2(0, 0);
        glm::ivec2 spawnerSize = glm::ivec2(1, 1);
        glm::ivec2 roomBottomLeftTilePos = glm::ivec2(0, 0);
        glm::ivec2 roomSize = glm::ivec2(1, 1);
        int monsterCounts[MonsterType::NONE] = {0, 0, 0};
        std::vector<glm::ivec2> occupiedTiles;
    };

    MonsterSpawnerComponent(MonsterSpawnerData p_msd);
    ~MonsterSpawnerComponent();
    
    void Init();
    void Update(float p_delta);

private:
    LabyrinthManager* m_pLBMG = nullptr;
    MonsterSpawnerData m_MSData;
    static wolf::RNG s_RNG;

    void SpawnMonsters();
};