//-----------------------------------------------------------------------------
// File: MonsterSpawnerComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Spawns monsters in a room
//-----------------------------------------------------------------------------

#include "MonsterSpawnerComponent.h"

#include <ChestInventoryComponent.h>
#include <ColliderComponent.h>
#include <DispensaryInventoryComponent.h>
#include <GorgonController.h>
#include <HarpyController.h>
#include <MinitaurController.h>
#include <NPCComponent.h>
#include <PlayerController.h>
#include <TrappedChestComponent.h>

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

    // std::cout << "worldPos: " << worldPos.x << ", y: " << worldPos.y << std::endl;
    HandleBoundLines();
    
    // if player is on spawner tile, spawn monsters
    glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(playerControllerComp->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    if(playerTilePos == glm::ivec2(-1)) return;;
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
    

    QueryOccupiedTiles();
    QueryAvailableTiles();

    // Load enemy data
    EnemyDataLoader loader;
    loader.LoadAllEnemyData("data/enemies.yaml");
    EnemyData minitaurData = loader.LoadEnemyData("minitaur");
    EnemyData harpyData = loader.LoadEnemyData("harpy");
    EnemyData gorgonData = loader.LoadEnemyData("gorgon");
    MinitaurBuilder minitaurBuilder(GetGameObject()->GetScene());
    HarpyBuilder harpyBuilder(GetGameObject()->GetScene());
    GorgonBuilder gorgonBuilder(GetGameObject()->GetScene());
    
    // Spawn gorgons
    int gorgonCount = m_MSData.gorgonCount;
    if(gorgonCount > 0)
    {
        for(int i = 0; i < gorgonCount; i++)
        {
            // Skip spawning if no tile left available
            if(m_vAvailableTiles.size() <= 0)
            {
                break;
                printf("GS - NO EMPTY TILES\n");
            }
            // Update available tiles count

            // Generate random tile position
            int randomIndex = s_RNG.NextInt(0, m_vAvailableTiles.size() - 1);
            glm::ivec2 pos = m_vAvailableTiles.at(randomIndex);
            
            // Mark tile as occupied
            m_vOccupiedTiles.push_back(pos);
            m_vAvailableTiles.erase(m_vAvailableTiles.begin() + randomIndex);

            // Calculate world position
            glm::vec2 worldPos = m_pLBMG->GetWorldPosition(pos);
            worldPos += glm::vec2(0.5f * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE);

            wolf::GameObject& gorgon = gorgonBuilder.BuildGorgon(gorgonData, worldPos);
            gorgon.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
        }
    }

    // Spawn minitaurs
    int minitaurCount = m_MSData.minitaurCount;
    
    if(minitaurCount > 0)
    {
        for(int i = 0; i < minitaurCount; i++)
        {
            // Skip spawning if no tile left available
            if(m_vAvailableTiles.size() <= 0)
            {
                
                printf("MS - NO EMPTY TILES\n");
                break;
            }

            // Generate random tile position
            int randomIndex = s_RNG.NextInt(0, m_vAvailableTiles.size() - 1);
            glm::ivec2 pos = m_vAvailableTiles.at(randomIndex);
            
            // Mark tile as occupied
            m_vOccupiedTiles.push_back(pos);
            m_vAvailableTiles.erase(m_vAvailableTiles.begin() + randomIndex);
            
            glm::vec2 worldPos = m_pLBMG->GetWorldPosition(glm::vec2(pos));
            worldPos += glm::vec2(0.5f * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE);

            wolf::GameObject& minitaur = minitaurBuilder.BuildMinitaur(minitaurData, worldPos);
            minitaur.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
        }
    }

    // Spawn harpies
    int harpyCount = m_MSData.harpyCount;   
    if(harpyCount > 0)
    {
        for(int i = 0; i < harpyCount; i++)
        {
            glm::vec2 pos;
            pos.x = s_RNG.NextInt(m_MSData.spawnerTilePos.x, m_MSData.spawnerTilePos.x + m_MSData.spawnerSize.x - 1);
            pos.y = s_RNG.NextInt(m_MSData.spawnerTilePos.y, m_MSData.spawnerTilePos.y + m_MSData.spawnerSize.y - 1);

            pos = m_pLBMG->GetWorldPosition(pos);

            wolf::GameObject& harpy = harpyBuilder.BuildHarpy(harpyData, pos);
            harpy.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));  
        }
    } 
}

void MonsterSpawnerComponent::QueryOccupiedTiles()
{
    int left = m_MSData.spawnerSize.x;
    int right = m_MSData.spawnerSize.x + m_MSData.spawnerSize.x - 1;
    int bottom = m_MSData.spawnerSize.y;
    int top = m_MSData.spawnerSize.y + m_MSData.spawnerSize.y - 1;

    // Get chunk IDs of lef-bottom & right-top corners
    glm::ivec2 chunkIDlb = m_pLBMG->GetChunkID(m_pLBMG->GetWorldPosition(glm::vec2(left, bottom)));  
    glm::ivec2 chunkIDrt = m_pLBMG->GetChunkID(m_pLBMG->GetWorldPosition(glm::vec2(right, top)));

    // Horizontal
    for(int i = chunkIDlb.x; i <= chunkIDrt.x; i++)
    {
        // Vertical
        for(int j = chunkIDlb.y; j <= chunkIDrt.y; j++)
        {
            // Get chunk data 
            glm::ivec2 chunkID = glm::ivec2(i, j);
            wolf::GameObject* chunk =  m_pLBMG->GetChunk(chunkID);
            std::vector<wolf::GameObject*> chunkObjs = chunk->GetChildren();
            
            // Iterate through every object in a chunk
            for (wolf::GameObject* obj: chunkObjs)
            {
                glm::vec2 objPos = obj->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::ivec2 objTilePos = m_pLBMG->GetTilePosition(objPos);
                
                // Skip if object is outside of room
                if(
                    objTilePos.x < left     ||
                    objTilePos.x > right    ||
                    objTilePos.y < bottom   ||
                    objTilePos.y > top      
                )
                {
                    continue;
                }

                // Skip if object tile position is already marked as occupied
                if(std::find(m_vOccupiedTiles.begin(), m_vOccupiedTiles.end(), objTilePos) != m_vOccupiedTiles.end())
                {
                    continue;
                }
                
                // Mark tile as occupied if object has any of the following components
                if(obj->HasAny<ChestInventoryComponent, ColliderComponent, DispensaryInventoryComponent, MinitaurController, NPCComponent, PlayerController, TrappedChestComponent>())
                {
                    std::cout <<"id: " << obj->GetID() << std::endl;
                    m_vOccupiedTiles.push_back(objTilePos);
                }

            }   
        }   
    }
}

void MonsterSpawnerComponent::QueryAvailableTiles()
{
    glm::ivec2 spawnerSize = m_MSData.spawnerSize;
    glm::ivec2 spawnerTilePos = m_MSData.spawnerTilePos;
    
    // Horizontal
    for(int i = spawnerTilePos.x; i < spawnerTilePos.x + spawnerSize.x; i++)
    {
        // Vertical
        for(int j = spawnerTilePos.y; j < spawnerTilePos.y + spawnerSize.y; j++)
        {
            glm::ivec2 tilePos = glm::ivec2(i, j);

            // If tile is not occupied, add to available vector
            if(std::find(m_vOccupiedTiles.begin(), m_vOccupiedTiles.end(), tilePos) == m_vOccupiedTiles.end())
            {
                m_vAvailableTiles.push_back(tilePos);
            }
        }
    }
}

void MonsterSpawnerComponent::HandleBoundLines()
{
    glm::ivec2 roomSize = m_MSData.spawnerSize * LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    glm::vec2 worldPos = m_pLBMG->GetWorldPosition(m_MSData.spawnerTilePos);

    glm::vec2 lb = worldPos;
    glm::vec2 lt = lb + glm::vec2(0.0f, roomSize.y);
    glm::vec2 rb = lb + glm::vec2(roomSize.x, 0.0f);
    glm::vec2 rt = lb + glm::vec2(roomSize);
 
    glm::vec4 colour = glm::vec4(1, 1, 0, 1);

    GLShapesRenderer::GetInstance()->AddLine(
                                            {lb.x, lb.y, colour.r, colour.g, colour.b, colour.a},
                                            {lt.x, lt.y, colour.r, colour.g, colour.b, colour.a}
                                            );

    GLShapesRenderer::GetInstance()->AddLine(
                                            {lt.x, lt.y, colour.r, colour.g, colour.b, colour.a},
                                            {rt.x, rt.y, colour.r, colour.g, colour.b, colour.a}
                                            );

    GLShapesRenderer::GetInstance()->AddLine(
                                            {rt.x, rt.y, colour.r, colour.g, colour.b, colour.a},
                                            {rb.x, rb.y, colour.r, colour.g, colour.b, colour.a}
                                            );

    GLShapesRenderer::GetInstance()->AddLine(
                                            {rb.x, rb.y, colour.r, colour.g, colour.b, colour.a},
                                            {lb.x, lb.y, colour.r, colour.g, colour.b, colour.a}
                                            );
}