//-----------------------------------------------------------------------------
// File: PortalTileManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------
#include "PortalTileManager.h"

#include <AttackDamageComponent.h>
#include <GorgonController.h>
#include <HarpyController.h>
#include <MinitaurController.h>
#include <PlayerController.h>
#include <ThrowableObjectComponent.h>
#include <VelocityComponent.h>

PortalTileManager* PortalTileManager::s_pPTMG = nullptr;

//-------------------//
//  MANAGER METHODS  //
//-------------------//

void PortalTileManager::CreateInstance(LabyrinthManager* p_lbmg)
{
    if(s_pPTMG == nullptr)
    {
        s_pPTMG = new PortalTileManager(p_lbmg);
    }
}

void PortalTileManager::DestroyInstance()
{
    if(s_pPTMG != nullptr)
    {
        while(s_pPTMG->m_vPortalTilePairs.size() > 0)
        {
            std::pair<PortalTile*, PortalTile*> pair = s_pPTMG->m_vPortalTilePairs.back();
            s_pPTMG->m_vPortalTilePairs.pop_back();
            PortalTile::DeletePair(pair.first, pair.second);
        }

        s_pPTMG->m_vPortalTilePairs.clear();

        s_pPTMG->m_pLBMG = nullptr;

        delete s_pPTMG;
        s_pPTMG = nullptr;
    }
}

PortalTileManager* PortalTileManager::GetInstance()
{
    return s_pPTMG;
}

void PortalTileManager::Update(float p_dt)
{
    // Get pair
    for(auto portalTilePair: m_vPortalTilePairs)
    {
        PortalTile* pt1 = portalTilePair.first;
        PortalTile* pt2 = portalTilePair.second;

        // Update each portal tile
        pt1->Update(p_dt);
        pt2->Update(p_dt);
    }
}
void PortalTileManager::CreatePortalTilePair(glm::ivec2 p_tile_pos_1, glm::ivec2 p_tile_pos_2)
{
    // Return immediately if tiles are invalid
    if(!IsValidTile(p_tile_pos_1) || !IsValidTile(p_tile_pos_2))
    {
        return;
    }

    // Create portal tile pair
    std::pair<PortalTileManager::PortalTile*, PortalTileManager::PortalTile*> portalTilePair = PortalTile::CreatePair(p_tile_pos_1, p_tile_pos_2, m_pLBMG);
    m_vPortalTilePairs.push_back(portalTilePair);
}


PortalTileManager::PortalTileManager(LabyrinthManager* p_lbmg)
{
    m_pLBMG = p_lbmg;
}

PortalTileManager::~PortalTileManager()
{
    m_pLBMG = nullptr;
}

bool PortalTileManager::IsValidTile(glm::ivec2 p_tile_pos) const
{
    // Check if tile is out of bounds
    return (p_tile_pos.x >= 0 && p_tile_pos.y >= 0);
}

//------------------//
//  STRUCT METHODS  //
//------------------//

std::pair<PortalTileManager::PortalTile*, PortalTileManager::PortalTile*> PortalTileManager::PortalTile::CreatePair(glm::ivec2 p_tile_pos_1, glm::ivec2 p_tile_pos_2, LabyrinthManager* p_lbmg)
{
    PortalTile* portalTile1 = new PortalTile(p_tile_pos_1, p_lbmg);
    PortalTile* portalTile2 = new PortalTile(p_tile_pos_2, p_lbmg);
    portalTile1->m_pSiblingPortalTile = portalTile2;
    portalTile2->m_pSiblingPortalTile = portalTile1;
    return std::pair(portalTile1, portalTile2);
}

void PortalTileManager::PortalTile::DeletePair(PortalTile* p_protal_tile_1, PortalTile* p_protal_tile_2)
{
    if(
    p_protal_tile_1 == nullptr                          ||
    p_protal_tile_2 == nullptr                          ||
    p_protal_tile_1->GetSibling() != p_protal_tile_2    || 
    p_protal_tile_2->GetSibling() != p_protal_tile_1
    ) 
    {
    return;
    }

    delete p_protal_tile_1;
    delete p_protal_tile_2;
}

PortalTileManager::PortalTile::PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg)
{
    // Initialise member variables
    m_vTilePos = p_tile_pos;
    m_bIsActive = false;
    m_vChunkID = p_lbmg->GetChunkID(p_lbmg->GetWorldPosition(p_tile_pos));
    m_pChunk = p_lbmg->GetChunk(m_vChunkID);
    m_pLabyrinthManager = p_lbmg;

    // Get playerobject
    for (auto&& [_, playerController] : p_lbmg->GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pPlayer = playerController.GetGameObject();
        break;
    }

    // Create sprite object
    m_pPortalTileSpriteObj = &p_lbmg->GetGameObject()->GetScene().CreateObject2D();
    m_pPortalTileSpriteObj->GetComponent<wolf::Transform2D>()->SetPosition(p_lbmg->GetWorldPosition(m_vTilePos));
    m_pPortalTileSpriteObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f, 3.0f));
    wolf::Sprite2D* sprite = &m_pPortalTileSpriteObj->AddComponent<wolf::Sprite2D>("data/textures/tile_hermes_portal.png");
}

PortalTileManager::PortalTile::~PortalTile()
{
    m_pSiblingPortalTile = nullptr;
    m_pArrival = nullptr;
    m_pChunk = nullptr;
    m_pLabyrinthManager = nullptr;
    wolf::Scene* scene = &m_pPortalTileSpriteObj->GetScene();
    scene->DeleteObject(m_pPortalTileSpriteObj->GetID());
}

void PortalTileManager::PortalTile::Update(float p_dt)
{
    bool isChunkActive = m_pLabyrinthManager->IsChunkActive(GetChunkID());
    // Return if chunk is inactive
    if(!isChunkActive) return;
    
    // If portal tile is inactive
    if(!IsActive())
    {
        // If reference to player object exists
        if(m_pPlayer != nullptr)
        {
            glm::vec2 playerPos = m_pPlayer->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::ivec2 playerTilePos  = m_pLabyrinthManager->GetTilePosition(playerPos);
            // std::cout << "PlayerTilePos - x: " << playerTilePos.x << ", y: " << playerTilePos.y << std::endl;
            // std::cout << "PortalTilePos - x: " << m_vTilePos.x << ", y: " << m_vTilePos.y << std::endl;

            // If player is within vicinity
            if
            (
                abs(playerTilePos.x - m_vTilePos.x) <= 1 &&
                abs(playerTilePos.y - m_vTilePos.y) <= 1
            )
            {
                SetActive(true);   
                printf("ACTIVATE\n");
            }
        }
    }
    // If portal tile is active
    else
    {
        // Return if sibling is not active
        if(!m_pSiblingPortalTile->IsActive()) return;

        // Get own arrival data if exists
        if(m_pArrival != nullptr)
        {
            glm::vec2 arrivalPos = m_pArrival->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::ivec2 arrivalTilePos = m_pLabyrinthManager->GetTilePosition(arrivalPos);
            
            // If arrival has stepped out of portal tile, remove reference
            if(arrivalTilePos != m_vTilePos)
            {
                m_pArrival = nullptr;
            }
            return;
        }

        // Check player
        CheckTeleport(m_pPlayer);

        // Check all objects within the portal tile chunk
        for(wolf::GameObject* obj : GetChunk()->GetChildren())
        {
            CheckTeleport(obj);
        }
    }
}

glm::ivec2 PortalTileManager::PortalTile::GetTilePos() const
{
    return m_vTilePos;
}

bool PortalTileManager::PortalTile::IsActive() const
{
    return m_bIsActive;
}

PortalTileManager::PortalTile* PortalTileManager::PortalTile::GetSibling() const
{
    return m_pSiblingPortalTile;
}

wolf::GameObject* PortalTileManager::PortalTile::GetArrival() const
{
    return m_pArrival;
}

wolf::GameObject* PortalTileManager::PortalTile::GetChunk() const
{
    return m_pChunk;
}

glm::ivec2 PortalTileManager::PortalTile::GetChunkID() const
{
    return m_vChunkID;
}

void PortalTileManager::PortalTile::SetActive(bool p_active)
{
    m_bIsActive = p_active;
}

void PortalTileManager::PortalTile::SetArrival(wolf::GameObject* p_arrival)
{
    m_pArrival = p_arrival;
}

void PortalTileManager::PortalTile::CheckTeleport(wolf::GameObject* p_obj)
{
    // Skip if object does not have VelocityComponent 
    if(!p_obj->HasAny<VelocityComponent>()) return;

    glm::ivec2 portalTilePos = GetTilePos();

    // Calculate tile position of object
    wolf::Transform2D* objTransform = p_obj->GetComponent<wolf::Transform2D>();
    glm::vec2 objPos = objTransform->GetGlobalPosition();
    glm::ivec2 objTilePos = m_pLabyrinthManager->GetTilePosition(objPos);

    // Skip if object is not on portal
    if(objTilePos.x != portalTilePos.x || objTilePos.y != portalTilePos.y) return;

    // Get sibling portal tile & its arrival object
    PortalTile* sibling = GetSibling();
    wolf::GameObject* siblingArrival = sibling->GetArrival();
    
    // If there is no sibling arrival or object is harpy or projectile, teleport
    if(siblingArrival == nullptr || p_obj->HasAny<HarpyController, AttackDamageComponent>())
    {
        Teleport(p_obj);
        return;
    }
    // If sibling has an arrival
    else
    {
        // Telefrag the sibling arrival if it has any of the components below
        
        // Gorgon
        if(siblingArrival->HasAny<GorgonController>())
        {
            GorgonController* gc = siblingArrival->GetComponent<GorgonController>();
            gc->ChangeState(EnemyController::EnemyState::DEATH);
            // Teleport object
            Teleport(p_obj);
            return;
        }
        //Minitaur
        if(siblingArrival->HasAny<MinitaurController>())
        {
            MinitaurController* mc = siblingArrival->GetComponent<MinitaurController>();
            mc->ChangeState(EnemyController::EnemyState::DEATH);
            // Teleport object
            Teleport(p_obj);
            return;
        }
        // Throwable
        if(siblingArrival->HasAny<ThrowableObjectComponent>())
        {
            m_pLabyrinthManager->GetGameObject()->GetScene().DeleteObject(siblingArrival->GetID());
            // Teleport object
            Teleport(p_obj);
            return;
        }
    }
}

void PortalTileManager::PortalTile::Teleport(wolf::GameObject* p_obj)
{
    wolf::Transform2D* objTransform = p_obj->GetComponent<wolf::Transform2D>();

    // Teleport object
    objTransform->SetPosition(m_pLabyrinthManager->GetWorldPosition(m_pSiblingPortalTile->GetTilePos()) + SPAWN_OFFSET);
            
    // Return if object is a harpy
    if(p_obj->HasAny<HarpyController>()) return;

    // Set object as new arrival
    m_pSiblingPortalTile->SetArrival(p_obj);
}