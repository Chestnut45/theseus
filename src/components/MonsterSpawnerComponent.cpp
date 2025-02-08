//-----------------------------------------------------------------------------
// File: MonsterSpawnerComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Spawns monsters in a room
// Note: Game object must NOT move for the component to work
//-----------------------------------------------------------------------------

#include "MonsterSpawnerComponent.h"
#include <PlayerController.h>
#include <EnemyDataLoader.h>
#include <GLShapesRenderer.h>
#include <GorgonBuilder.h>
#include <HarpyBuilder.h>
#include <MinitaurBuilder.h>

wolf::RNG MonsterSpawnerComponent::s_RNG;

MonsterSpawnerComponent::MonsterSpawnerComponent(MonsterSpawnerData p_msd)
{
    m_MSData = p_msd;
    glm::vec2 worldPos = m_pLBMG->GetWorldPosition(m_MSData.spawnerTilePos);
}

MonsterSpawnerComponent::~MonsterSpawnerComponent()
{
    m_pLBMG = nullptr;
}

void MonsterSpawnerComponent::Init()
{
    for (auto&& [_, lbmg] : GetGameObject()->GetScene().Each<LabyrinthManager>())
    {
        m_pLBMG = &lbmg;
        break;
    }
}

void MonsterSpawnerComponent::Update(float p_delta)
{
    // Update if on-step trigger
    if(m_MSData.spawnTrigger == SpawnTrigger::ON_STEP)
    {
        // get player controller
        PlayerController* playerControllerComp = nullptr;
        for (auto&& [_, pcComp] : GetGameObject()->GetScene().Each<PlayerController>())
        {
            playerControllerComp = &pcComp;
            break;
        }

        glm::vec2 worldPos = m_pLBMG->GetWorldPosition(m_MSData.spawnerTilePos);
        // std::cout << "worldPos: " << worldPos.x << ", y: " << worldPos.y << std::endl;
        GLShapesRenderer::GetInstance()->AddQuad(
                                                {worldPos.x, worldPos.y, 0, 1, 0, 1}, 
                                                m_MSData.spawnerSize.x * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE,
                                                m_MSData.spawnerSize.y * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE
                                                );
        
        // if player is on spawner tile, spawn monsters
        glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(playerControllerComp->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
        if(
            playerTilePos.x < m_MSData.spawnerTilePos.x                                 ||
            playerTilePos.x > m_MSData.spawnerTilePos.x + (m_MSData.spawnerSize.x - 1)  ||
            playerTilePos.y < m_MSData.spawnerTilePos.y                                 ||
            playerTilePos.y > m_MSData.spawnerTilePos.y + (m_MSData.spawnerSize.y - 1)
        )
        {
            return;
        }
        else
        {
            SpawnMonsters();
            this->GetGameObject()->GetScene().DeleteObject(this->GetGameObject()->GetID());
            return;
        }
    }
    
}

void MonsterSpawnerComponent::CallSpawnMonsters()
{
    if(m_MSData.spawnTrigger != SpawnTrigger::ON_CALL)
    {
        return;
    }

    SpawnMonsters();
}

void MonsterSpawnerComponent::SpawnMonsters()
{
    // get player controller
    PlayerController* playerControllerComp = nullptr;
    for (auto&& [_, pcComp] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        playerControllerComp = &pcComp;
        break;
    }
    glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(playerControllerComp->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    
    // Create vector of occupied tiles
    std::vector<glm::ivec2> occupiedTiles;
    occupiedTiles.push_back(playerTilePos);
    
    // Load enemy data
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");
    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    EnemyData harpyData = loader.LoadEnemyData("harpy");
    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    MinitaurBuilder minitaurBuilder(GetGameObject()->GetScene());
    HarpyBuilder harpyBuilder(GetGameObject()->GetScene());
    GorgonBuilder gorgonBuilder(GetGameObject()->GetScene());

    int roomArea = m_MSData.roomSize.x * m_MSData.roomSize.y;

    // Spawn harpies
    int harpyCount = m_MSData.aMonsterCounts[MonsterSpawnerComponent::MonsterType::HARPY];
    if(harpyCount > 0)
    {
        for(int i = 0; i < harpyCount; i++)
        {
            glm::vec2 pos;
            pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x);
            pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y);
            
            pos.x = m_MSData.spawnerTilePos.x;
            pos.y = m_MSData.spawnerTilePos.y;
            pos = m_pLBMG->GetWorldPosition(pos);

            wolf::GameObject& harpy = harpyBuilder.BuildHarpy(harpyData, pos);
            harpy.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
        }
    }
}