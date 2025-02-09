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
    s_RNG.NextInt(1, 100);
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
    

    m_vOccupiedTiles.push_back(playerTilePos);
    
    // Load enemy data
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");
    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    EnemyData harpyData = loader.LoadEnemyData("harpy");
    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    MinitaurBuilder minitaurBuilder(GetGameObject()->GetScene());
    HarpyBuilder harpyBuilder(GetGameObject()->GetScene());
    GorgonBuilder gorgonBuilder(GetGameObject()->GetScene());

    int availableTiles = m_MSData.roomSize.x * m_MSData.roomSize.y - 1;
    
    // Spawn gorgons
    int gorgonCount = m_MSData.monsterCounts[MonsterSpawnerComponent::MonsterType::GORGON];
    if(gorgonCount > 0)
    {
        for(int i = 0; i < gorgonCount; i++)
        {
            // Skip spawning if no tile left available
            if(availableTiles <= 0)
            {
                break;
                printf("NO EMPTY TILES\n");
            }
            // Update available tiles count
            availableTiles--;

            // Generate random tile position
            glm::ivec2 pos;
            pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1);
            pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1);    
            pos.x = m_MSData.spawnerTilePos.x;
            pos.y = m_MSData.spawnerTilePos.y;

            // Regenerate until new tile position does not match an occupied one
            while(std::find(m_vOccupiedTiles.begin(), m_vOccupiedTiles.end(), pos) != m_vOccupiedTiles.end())
            {
                pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1);
                pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1); 
            }
            // Add new tile position to list of occupied tiles
            m_vOccupiedTiles.push_back(pos);

            // Calculate world position
            glm::vec2 worldPos = m_pLBMG->GetWorldPosition(pos);
            worldPos += glm::vec2(0.5f * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE);

            wolf::GameObject& gorgon = gorgonBuilder.BuildGorgon(gorgonData, worldPos);
            gorgon.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
        }
    }

    // Spawn minitaurs
    int minitaurCount = m_MSData.monsterCounts[MonsterSpawnerComponent::MonsterType::MINITAUR];
    
    if(minitaurCount > 0)
    {
        for(int i = 0; i < minitaurCount; i++)
        {
            // Skip spawning if no tile left available
            if(availableTiles <= 0)
            {
                
                printf("NO EMPTY TILES\n");
                break;
            }

            // Update available tiles count
            availableTiles--;

            // Generate random tile position
            glm::ivec2 pos;
            pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1);
            pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1);

            // Regenerate until new tile position does not match an occupied one
            while(std::find(m_vOccupiedTiles.begin(), m_vOccupiedTiles.end(), pos) != m_vOccupiedTiles.end())
            {
                pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1);
                pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1); 
            }
            m_vOccupiedTiles.push_back(pos);
            glm::vec2 worldPos = m_pLBMG->GetWorldPosition(glm::vec2(pos));
            worldPos += glm::vec2(0.5f * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE);

            wolf::GameObject& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, worldPos);
            minitaur.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
        }
    }

    // Spawn harpies
    int harpyCount = m_MSData.monsterCounts[MonsterSpawnerComponent::MonsterType::HARPY];   
    if(harpyCount > 0)
    {
        for(int i = 0; i < harpyCount; i++)
        {
            glm::vec2 pos;
            pos.x = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.x, m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1);
            pos.y = s_RNG.NextInt(m_MSData.roomBottomLeftTilePos.y, m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1);

            pos = m_pLBMG->GetWorldPosition(pos);

            wolf::GameObject& harpy = harpyBuilder.BuildHarpy(harpyData, pos);
            harpy.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));  
        }
    } 
}

void MonsterSpawnerComponent::QueryOccupiedTiles()
{
    int left = m_MSData.roomBottomLeftTilePos.x;
    int right = m_MSData.roomBottomLeftTilePos.x + m_MSData.roomSize.x - 1;
    int bottom = m_MSData.roomBottomLeftTilePos.y;
    int top = m_MSData.roomBottomLeftTilePos.y + m_MSData.roomSize.y - 1;

    // Get chunk IDs of lef-bottom & right-top corners
    glm::ivec2 chunkIDlb = m_pLBMG->GetChunkID(m_pLBMG->GetWorldPosition(glm::vec2(left, bottom)));  
    glm::ivec2 chunkIDrt = m_pLBMG->GetChunkID(m_pLBMG->GetWorldPosition(glm::vec2(right, top)));

    // Horizontal
    for(int i = chunkIDlb.x; i <= chunkIDrt.x; i++)
    {
        // Vertical
        for(int j = chunkIDlb.y; j <= chunkIDrt.y; j++)
        {
            glm::ivec2 chunkID = glm::ivec2(i, j);
            wolf::GameObject* chunk =  m_pLBMG->GetChunk(chunkID);
            std::vector<wolf::GameObject*> chunkChildren = chunk->GetChildren();
            
            // Iterate through every object in a chunk
            for (wolf::GameObject* child: chunkChildren)
            {

            }
        }   
    }
}