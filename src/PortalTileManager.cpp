//-----------------------------------------------------------------------------
// File: PortalTileManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------
#include "PortalTileManager.h"

#include <AttackDamageComponent.h>
#include <HealthComponent.h>
#include <PlayerController.h>
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

    // for(auto portalTilePair: m_vPortalTilePairs)
    // {
    //     PortalTile* ptPair[2] = { portalTilePair.first,  portalTilePair.second };
    //     for(int i = 0; i < 2; i++)
    //     {
    //         PortalTile* pt = ptPair[i];

    //         glm::vec2 playerPos = m_pPlayer->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    //         glm::ivec2 playerTilePos  = m_pLBMG->GetTilePosition(playerPos);
    //         if
    //         (
    //             abs(playerTilePos.x - pt->GetTilePos().x) <= 1 &&
    //             abs(playerTilePos.y - pt->GetTilePos().y) <= 1
    //         )
    //         {
    //             RenderCollectPrompt();
    //             if(wolf::Input::IsKeyJustDown(GLFW_KEY_E))
    //             {
    //                 wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, GetTilePos()));
    //                 if(GetSibling() != nullptr)
    //                 {
    //                     wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, GetTilePos()));
    //                     //DeletePair(this, GetSibling());
    //                 }
    //                 else
    //                 {
    //                     //DeleteAvailablePortalTile(this);
    //                 }
    //             }
    //         }
    //     }    
    // }

    
    if(m_pAvailablePortalTile != nullptr)
    {
        m_pAvailablePortalTile->CheckPlayerCollection();
        m_pAvailablePortalTile->Update(p_dt);
    }
}

bool PortalTileManager::CreatePortalTile(glm::ivec2 p_tile_pos)
{
    if(!IsValidTile(p_tile_pos)) return false;

    PortalTile* portalTile = PortalTile::CreatePortalTile(p_tile_pos, m_pLBMG, m_pAvailablePortalTile);
    if(m_pAvailablePortalTile == nullptr)
    {
        m_pAvailablePortalTile = portalTile;
    }
    else
    {
        m_vPortalTilePairs.push_back(std::pair(portalTile, portalTile->GetSibling()));
        m_pAvailablePortalTile = nullptr;
    }
    return true;
    
}

PortalTileManager::PortalTileManager(LabyrinthManager* p_lbmg)
{
    wolf::EventManager::AddListener<DestroyPlaceableEvent, PortalTileManager, &PortalTileManager::HandleDestroyPlaceableEvent>(*this);
    m_pLBMG = p_lbmg;
    for (auto&& [_, playerController] : p_lbmg->GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pPlayer = playerController.GetGameObject();
        break;
    }
}

PortalTileManager::~PortalTileManager()
{
    wolf::EventManager::RemoveListener<DestroyPlaceableEvent, PortalTileManager, &PortalTileManager::HandleDestroyPlaceableEvent>(*this);
    while(m_vPortalTilePairs.size() > 0)
    {
        std::pair<PortalTile*, PortalTile*> pair = m_vPortalTilePairs.back();
        m_vPortalTilePairs.pop_back();
        PortalTile::DeletePair(pair.first, pair.second);
    }
   
    if(m_pAvailablePortalTile != nullptr)
    {    
        PortalTile::DeleteAvailablePortalTile(m_pAvailablePortalTile);
        m_pAvailablePortalTile = nullptr;
    }

    m_vPortalTilePairs.clear();    
    m_pLBMG = nullptr;
    m_pPlayer = nullptr;
}

void PortalTileManager::HandleDestroyPlaceableEvent(const DestroyPlaceableEvent& p_event)
{
    if(p_event.pcTpye != PlaceableType::PORTAL) return;

    if(m_pAvailablePortalTile != nullptr && m_pAvailablePortalTile->GetTilePos() == p_event.tilePos)
    {
        printf("PT1\n");
        PortalTile::DeleteAvailablePortalTile(m_pAvailablePortalTile);
        m_pAvailablePortalTile = nullptr;
        
        return;
    }
    
    for(int i = 0; i < m_vPortalTilePairs.size(); i++)
    {
        PortalTile* pt1 = m_vPortalTilePairs.at(i).first;
        PortalTile* pt2 = m_vPortalTilePairs.at(i).second;    
        
        if(pt1->GetTilePos() == p_event.tilePos || pt2->GetTilePos() == p_event.tilePos)
        {
            
            printf("PT2\n");
            m_vPortalTilePairs.at(i).first = nullptr;
            m_vPortalTilePairs.at(i).second = nullptr;
            m_vPortalTilePairs.erase(m_vPortalTilePairs.begin() + i);
            PortalTile::DeletePair(pt1, pt2);
            return;
        }
    }
}

bool PortalTileManager::IsValidTile(glm::ivec2 p_tile_pos)
{
    // Check if tile is out of bounds
    return (p_tile_pos.x >= 0 && p_tile_pos.y >= 0);
}


void PortalTileManager::RenderCollectPrompt()
{
    // Set screen-space position for the pickup prompt
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 promptPosition = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.8f);  // Centered horizontally, lower portion vertically

    ImGui::SetNextWindowPos(promptPosition, ImGuiCond_Always, ImVec2(0.5f, 0.5f));  // Centered alignment
    ImGui::SetNextWindowBgAlpha(0.85f);

    // Pulse color and size animation for visual feedback
    float alphaPulse = 0.6f + 0.4f * sin(ImGui::GetTime() * 3.0f);
    ImVec4 glowColor = ImVec4(0.8f, 0.92f, 0.3f, alphaPulse); // Neon green glow

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Text, glowColor);
    ImGui::PushStyleColor(ImGuiCol_Border, glowColor);

    ImGui::Begin("PickUpPrompt", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Press E to pick up");
    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

//------------------//
//  STRUCT METHODS  //
//------------------//

PortalTileManager::PortalTile* PortalTileManager::PortalTile::CreatePortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg, PortalTile* p_sibling)
{
    PortalTile* portalTile = new PortalTile(p_tile_pos, p_lbmg);
    if(p_sibling != nullptr)
    {
        portalTile->m_pSiblingPortalTile = p_sibling;
        portalTile->m_pSiblingPortalTile->m_pSiblingPortalTile = portalTile;
        int index = PortalTileManager::GetInstance()->m_vPortalTilePairs.size() - 1;
        portalTile->m_iPairIndex = index;
        portalTile->m_pSiblingPortalTile->m_iPairIndex = index;
    }
    return portalTile;
}

void PortalTileManager::PortalTile::DeletePair(PortalTile* p_protal_tile_1, PortalTile* p_protal_tile_2)
{
    // Return if either pointer is nullptr, or siblings do not match
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

void PortalTileManager::PortalTile::DeleteAvailablePortalTile(PortalTile* p_protal_tile)
{
    // Return if pointer is nullptr or has a sibling
    if(
        p_protal_tile == nullptr                || 
        p_protal_tile->GetSibling() != nullptr
    )
    {
        return;
    }

    delete p_protal_tile;
}

PortalTileManager::PortalTile::PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg)
{
    // Initialise member variables
    m_vTilePos = p_tile_pos;
    m_bIsActive = false;
    m_vChunkID = p_lbmg->GetChunkID(p_lbmg->GetWorldPosition(p_tile_pos));
    m_pChunk = p_lbmg->GetChunk(m_vChunkID);
    m_pLabyrinthManager = p_lbmg;

    // Get player object
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
    sprite->SetTint(glm::vec3(0.5f));
}

PortalTileManager::PortalTile::~PortalTile()
{
    m_pSiblingPortalTile = nullptr;
    m_pChunk = nullptr;
    m_pLabyrinthManager = nullptr;
    m_pPlayer = nullptr;
    wolf::Scene* scene = &m_pPortalTileSpriteObj->GetScene();
    scene->DeleteObject(m_pPortalTileSpriteObj->GetID());
    m_pPortalTileSpriteObj = nullptr;
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

wolf::GameObjectID PortalTileManager::PortalTile::GetOccupantID() const
{
    return m_occupantID;
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

void PortalTileManager::PortalTile::SetOccupantID(wolf::GameObjectID p_occupant_id)
{
    m_occupantID = p_occupant_id;
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

            // std::cout << "PlayerTilePos - x: " << playerTilePos.x << ", y: " << playerTilePos.y << std::endl;
            // std::cout << "PortalTilePos - x: " << m_vTilePos.x << ", y: " << m_vTilePos.y << std::endl;

            glm::vec2 playerPos = m_pPlayer->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::ivec2 playerTilePos  = m_pLabyrinthManager->GetTilePosition(playerPos);
            // If player is within vicinity, activate
            if
            (
                abs(playerTilePos.x - m_vTilePos.x) <= 1 &&
                abs(playerTilePos.y - m_vTilePos.y) <= 1
            )
            {
                SetActive(true);
                m_pPortalTileSpriteObj->GetComponent<wolf::Sprite2D>()->SetTint(glm::vec3(1.0f));
            }
        }
    }
    // If portal tile is active
    else
    {        
        // Return if sibling is nullptr
        if(m_pSiblingPortalTile == nullptr) return;

        // Return if sibling is not active
        if(!m_pSiblingPortalTile->IsActive()) return;

        // Check if occupant exists
        wolf::GameObject* occupant = m_pLabyrinthManager->GetGameObject()->GetScene().GetObject(m_occupantID);
        if(occupant != nullptr)
        {   
            // Get occupant position data
            glm::vec2 occupantPos = occupant->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::ivec2 occupantTilePos = m_pLabyrinthManager->GetTilePosition(occupantPos);
            
            // If occupant has stepped out of portal tile, remove ID
            if(occupantTilePos != m_vTilePos)
            {
                m_occupantID = -1;
            }
        }
        else
        {
            // If occupant is deleted while still standing on portal tile, remove ID
            if(m_occupantID != -1)
            {
                m_occupantID = -1;
            }  
        }          

        // Check player
        CheckTeleport(m_pPlayer);

        // Check all objects within the portal tile chunk
        for(wolf::GameObject* obj : GetChunk()->GetChildren())
        {
            CheckTeleport(obj);
        }

        // Check projectiles
        for (auto&& [_, velocity, attackDamage] : m_pLabyrinthManager->GetGameObject()->GetScene().Each<VelocityComponent, AttackDamageComponent>())
        {
            CheckTeleport(attackDamage.GetGameObject());
        }
    }
}

void PortalTileManager::PortalTile::CheckTeleport(wolf::GameObject* p_obj)
{
    // Return if object is occupant
    if(p_obj->GetID() == m_occupantID) return;

    // Return if object does not have VelocityComponent 
    if(!p_obj->HasAny<VelocityComponent>()) return;

    glm::ivec2 portalTilePos = GetTilePos();

    // Calculate tile position of object
    wolf::Transform2D* objTransform = p_obj->GetComponent<wolf::Transform2D>();
    glm::vec2 objPos = objTransform->GetGlobalPosition();
    glm::ivec2 objTilePos = m_pLabyrinthManager->GetTilePosition(objPos);

    // Skip if object is not on portal
    if(objTilePos.x != portalTilePos.x || objTilePos.y != portalTilePos.y) return;

    // Get sibling portal tile & its occupant object
    PortalTile* sibling = GetSibling();
    wolf::GameObject* siblingOccupant = m_pLabyrinthManager->GetGameObject()->GetScene().GetObject(sibling->GetOccupantID());
    
    // If there is no sibling occupant, teleport
    if(siblingOccupant == nullptr)
    {
        Teleport(p_obj);
        return;
    }
    // If sibling has an occupant
    else
    {
        // If object is a projectile
        if(p_obj->HasAll<AttackDamageComponent, VelocityComponent>())
        {
            // Teleport object
            Teleport(p_obj);
            return;
        }

        // If sibling occupant is player
        if(siblingOccupant->HasAny<PlayerController>())
        {
            return;
        }

        // If sibling occupant has HealthComponent
        if(siblingOccupant->HasAny<HealthComponent>())
        {
            // Telefrag
            HealthComponent* hc = siblingOccupant->GetComponent<HealthComponent>();
            hc->Pierce(999999999.0f);

            // Teleport object
            Teleport(p_obj);
            return;
        }

        // Else
        else
        {
            // Delete sibling occupant
            m_pLabyrinthManager->GetGameObject()->GetScene().DeleteObject(siblingOccupant->GetID());
            
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

    // Set object as new occupant
    m_pSiblingPortalTile->SetOccupantID(p_obj->GetID());
}
void PortalTileManager::PortalTile::CheckPlayerCollection()
{
    glm::vec2 playerPos = m_pPlayer->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::ivec2 playerTilePos  = m_pLabyrinthManager->GetTilePosition(playerPos);
    if
    (
        abs(playerTilePos.x - m_vTilePos.x) <= 1 &&
        abs(playerTilePos.y - m_vTilePos.y) <= 1
    )
    {
        RenderCollectPrompt();
        if(wolf::Input::IsKeyJustDown(GLFW_KEY_E))
        {
            wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, GetTilePos()));
            if(GetSibling() != nullptr)
            {
                wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, GetTilePos()));
                //DeletePair(this, GetSibling());
            }
            else
            {
                //DeleteAvailablePortalTile(this);
            }
        }
    }
}
