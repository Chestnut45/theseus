//-----------------------------------------------------------------------------
// File: PortalTileManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------
#include "PortalTileManager.h"

#include <AttackDamageComponent.h>
#include <ColliderComponent.h>
#include <HealthComponent.h>
#include <LightComponent.h>
#include <ParticleComponent.h>
#include <PlayerController.h>
#include <VelocityComponent.h>
#include <BossController.h>
#include <HarpyController.h>
#include <W_Audio.h>
#include <W_RNG.h>
#include <W_TextureManager.h>


PortalTileManager* PortalTileManager::s_pPTMG = nullptr;

wolf::RNG PortalTileManager::PortalTile::s_rng;
//-------------------//
//  MANAGER METHODS  //
//-------------------//

void PortalTileManager::CreateInstance(LabyrinthManager* p_lbmg)
{
    if(s_pPTMG == nullptr)
    {
        s_pPTMG = new PortalTileManager(p_lbmg);
        s_pParticleTex = wolf::TextureManager::CreateTexture("data/textures/portal_particle.png");
        s_pParticleTex->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest, wolf::Texture::FilterMode::FM_Nearest);
    }
}

void PortalTileManager::DestroyInstance()
{
    if(s_pPTMG != nullptr)
    {
        delete s_pPTMG;
        s_pPTMG = nullptr;
        wolf::TextureManager::DestroyTexture(s_pParticleTex);
        s_pParticleTex = nullptr;
    }
}

PortalTileManager* PortalTileManager::GetInstance()
{
    return s_pPTMG;
}

void PortalTileManager::Update(float p_dt)
{
    // Iterate through protal tiles vector
    for(auto portalTile: m_vPortalTiles)
    {
        // Update each portal tile & its sibling
        portalTile->Update(p_dt);
        portalTile->GetSibling()->Update(p_dt);
    }
    
    
    glm::vec2 playerPos = m_pPlayer->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(playerPos);

    // If there is an available tile
    if(m_pAvailablePortalTile != nullptr)
    {
        // Update
        m_pAvailablePortalTile->Update(p_dt);
        
        // If the available portal tile is within collecting range of the player
        int range = m_pPlayer->GetComponent<PlayerController>()->GetPlaceableCollectingRange();
        if (abs(playerTilePos.x - m_pAvailablePortalTile->GetTilePos().x) <= range && abs(playerTilePos.y - m_pAvailablePortalTile->GetTilePos().y) <= range)
        {
            RenderCollectPrompt();

            // If the E key was just pressed, set the removal index to the available tile
            if (wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
                m_iRemovalIndex = -1;
            }
        }
    }

    // If available tile was not removed, then check coupled tiles for collection (Avoids double collection)
    if(m_iRemovalIndex == -2)
    {
        for (int i = 0; i < m_vPortalTiles.size(); i++) {
            // Get the portal & its sibling
            PortalTile* pt = m_vPortalTiles.at(i);
            PortalTile* ptSibling = pt->GetSibling();
            
            
            // If either portal tile is within collecting range of the player
            if (
                (abs(playerTilePos.x - pt->GetTilePos().x) <= 1 && abs(playerTilePos.y - pt->GetTilePos().y) <= 1)                  ||
                (abs(playerTilePos.x - ptSibling->GetTilePos().x) <= 1 && abs(playerTilePos.y - ptSibling->GetTilePos().y) <= 1)
            ) {
                RenderCollectPrompt();

                // If the E key was just pressed, set the removal index to the current index & break
                if ( wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
                    m_iRemovalIndex = i;
                    break;
                }
            }
        }
    }

    // If a tile was set for removal
    if(m_iRemovalIndex != -2)
    {    
        // If it was the available tile, trigger the event to attempt to retrieve it
        if(m_iRemovalIndex == -1)
        {
            wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, m_pAvailablePortalTile->GetTilePos()));
            
            return;
        }
        
        // If it was a coupled tile,  trigger two events to attempt to retrieve it & its sibling 
        else
        {
            PortalTile* pt = m_vPortalTiles.at(m_iRemovalIndex);
            PortalTile* sibling = pt->GetSibling();
            
            if(sibling != nullptr)
            {
                wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, sibling->GetTilePos()));
            }
            wolf::EventManager::TriggerEvent(RetrievePlaceableEvent(PlaceableType::PORTAL, pt->GetTilePos()));
        }

    }
}

bool PortalTileManager::CreatePortalTile(glm::ivec2 p_tile_pos)
{
    // Return if tile position is not valid
    if(!IsValidTile(p_tile_pos)) return false;

    // If there is currently not an available portal tile (this portal tile will be dangling without a sibling)
    if(m_pAvailablePortalTile == nullptr)
    {
        // Create the portal tile & set it as available
        PortalTile* portalTile = PortalTile::CreatePortalTile(p_tile_pos, m_pLBMG, nullptr);
        m_pAvailablePortalTile = portalTile;
    }
    
    // If there is an available tile
    else
    {
        // Create the portal tile& couple it with the avaliable portal tile
        PortalTile* portalTile = PortalTile::CreatePortalTile(p_tile_pos, m_pLBMG, m_pAvailablePortalTile);
        
        // Push the portal tile into the vector
        m_vPortalTiles.push_back(portalTile);
        
        // Set available portal tile to nullptr
        m_pAvailablePortalTile = nullptr;
    }
    return true;
}

bool PortalTileManager::IsTileOccupiedByAnotherPortalTile(glm::ivec2 p_tile_pos)
{
    // If occupied by an available portal tile, return true
    if(m_pAvailablePortalTile != nullptr && m_pAvailablePortalTile->GetTilePos() == p_tile_pos)
    {
        return true;
    }

    for(const auto portalTile : m_vPortalTiles)
    {
        // If occupied by a portal tile or its sibling, return true
        if(portalTile->GetTilePos() == p_tile_pos || portalTile->GetSibling()->GetTilePos() == p_tile_pos)
        {
            return true;
        }
    }

    return false;
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

    for (auto&& [_, boss] : p_lbmg->GetGameObject()->GetScene().Each<BossController>())
    {
        m_pBoss = boss.GetGameObject();
        break;
    }
}

PortalTileManager::~PortalTileManager()
{
    wolf::EventManager::RemoveListener<DestroyPlaceableEvent, PortalTileManager, &PortalTileManager::HandleDestroyPlaceableEvent>(*this);
    while(m_vPortalTiles.size() > 0)
    {
        PortalTile* pt = m_vPortalTiles.back();
        m_vPortalTiles.pop_back();
        PortalTile::DeletePortalAndSibling(pt);
    }
   
    if(m_pAvailablePortalTile != nullptr)
    {    
        PortalTile::DeleteAvailablePortalTile(m_pAvailablePortalTile);
        m_pAvailablePortalTile = nullptr;
    }

    m_vPortalTiles.clear();    
    m_pLBMG = nullptr;
    m_pPlayer = nullptr;
}

// Explanation:
//  RetrievePlaceableEvent is triggered by collecting a portal and PlayerInventory successfully added a portal item, thus triggering DestroyPlaceableEvent and this method is called
//  The method deletes the portal tile marked by the removal index (and its sibling, if any), then it sets the removal index to -2
//  If a portal has a sibling, then RetrievePlaceableEvent is also triggered for the sibling
//  When this method is called for a sibling tile, the removal index is already set to -2, and thus the method will do nothing, avoiding double deltion

void PortalTileManager::HandleDestroyPlaceableEvent(const DestroyPlaceableEvent& p_event)
{
    if(p_event.pcTpye != PlaceableType::PORTAL) return;

    // Delete available portal tile & remove its reference
    if(m_iRemovalIndex == -1)
    {
        PortalTile::DeleteAvailablePortalTile(this->m_pAvailablePortalTile);
        this->m_pAvailablePortalTile = nullptr;
    }

    // Delete portal tile & sibling & remove its reference in the vector
    else if(m_iRemovalIndex >= 0)
    {
        PortalTile::DeletePortalAndSibling(m_vPortalTiles.at(m_iRemovalIndex));
        m_vPortalTiles.erase(m_vPortalTiles.begin() + m_iRemovalIndex);
    }

    // Reset removal index to -2
    m_iRemovalIndex = -2;
    return;
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

    // If sibling is not nullptr, couple the two portal tiles together
    if(p_sibling != nullptr)
    {
        portalTile->m_pSiblingPortalTile = p_sibling;
        p_sibling->m_pSiblingPortalTile = portalTile;
        int index = PortalTileManager::GetInstance()->m_vPortalTiles.size() - 1;
    }
    return portalTile;
}

void PortalTileManager::PortalTile::DeletePortalAndSibling(PortalTile* p_protal_tile_1)
{
    // Return if portal tile is nullptr
    if(p_protal_tile_1 == nullptr) return;
    
    // Get the sibling portal tile
    PortalTile* sibling = p_protal_tile_1->GetSibling();
    
    // Decouple the portal tiles
    p_protal_tile_1->m_pSiblingPortalTile = nullptr;
    sibling->m_pSiblingPortalTile = nullptr;
    
    // Delete the portal tiles
    delete p_protal_tile_1;
    delete sibling;
    
}

void PortalTileManager::PortalTile::DeleteAvailablePortalTile(PortalTile* p_protal_tile)
{
    // Return if pointer is nullptr or has a sibling
    if(p_protal_tile == nullptr || p_protal_tile->GetSibling() != nullptr) return;

    // Delete the portal tile
    delete p_protal_tile;
}

PortalTileManager::PortalTile::PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg)
{
    // Initialise member variables
    m_vTilePos = p_tile_pos;
    m_vChunkID = p_lbmg->GetChunkID(p_lbmg->GetWorldPosition(p_tile_pos));
    m_pChunk = p_lbmg->GetChunk(m_vChunkID);
    m_pLabyrinthManager = p_lbmg;

    // Get player object
    for (auto&& [_, playerController] : p_lbmg->GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pPlayer = playerController.GetGameObject();
        break;
    }

    // Get boss object
    for (auto&& [_, boss] : p_lbmg->GetGameObject()->GetScene().Each<BossController>())
    {
        m_pBoss = boss.GetGameObject();
        break;
    }

    float scaledTileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;

    // Create sprite object
    m_pPortalTileSpriteObj = &p_lbmg->GetGameObject()->GetScene().CreateObject2D();
    m_pPortalTileSpriteObj->GetComponent<wolf::Transform2D>()->SetPosition(p_lbmg->GetWorldPosition(m_vTilePos));
    m_pPortalTileSpriteObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f, 3.0f));

    // Add 2D sprite component
    wolf::Sprite2D* sprite = &m_pPortalTileSpriteObj->AddComponent<wolf::Sprite2D>("data/textures/hermes_portal.png");
    sprite->SetTint(glm::vec3(1.0f));
    sprite->SetLayer(0);

    // Add particle component
    ParticleComponent* particleComponent = &m_pPortalTileSpriteObj->AddComponent<ParticleComponent>();
    particleComponent->LoadConfigFromYAML("data/particles/portal.yaml");

    // Add collider component
    ColliderComponent* colliderComponent = &m_pPortalTileSpriteObj->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, false, false);
    colliderComponent->AddColliderBox(glm::vec2(scaledTileSize), glm::vec2(0.0f, scaledTileSize));

    // Add light component
    wolf::GameObject* lightObj = &p_lbmg->GetGameObject()->GetScene().CreateObject2D();
    lightObj->GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(LabyrinthManager::TILE_SIZE * 0.5f, LabyrinthManager::TILE_SIZE * 0.5f));
    LightComponent* lightComponent = &lightObj->AddComponent<LightComponent>(glm::vec4(1.0f, 0.64f, 0.0f, 0.75f), scaledTileSize, true);
    m_pPortalTileSpriteObj->AddChild(*lightObj);
    lightComponent->Init();
    lightComponent->SetIgnoreWallTiles(true);

    m_pChunk->AddChild(*m_pPortalTileSpriteObj);

    m_emissionTimer.Start();
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

void PortalTileManager::PortalTile::SetOccupantID(wolf::GameObjectID p_occupant_id)
{
    m_occupantID = p_occupant_id;
}

void PortalTileManager::PortalTile::Update(float p_dt)
{
    bool isChunkActive = m_pLabyrinthManager->IsChunkActive(GetChunkID());
    
    // Return if chunk is inactive and we haven't started the bossfight yet
    if(!isChunkActive && !m_pLabyrinthManager->IsBossfightStarted()) return;
       
    // Emit particles (now with consistent speed)
    if(m_emissionTimer.Elapsed() > m_nextEmission)
    {
        m_nextEmission = s_rng.NextFloat(0.01f, 1.0f);
        m_emissionTimer.Restart();
        if (auto* pParticles = m_pPortalTileSpriteObj->GetComponent<ParticleComponent>())
        {
            pParticles->Emit(
                glm::vec2(
                    m_vTilePos.x * SCALED_TILE_SIZE + SCALED_TILE_SIZE * 0.5f,
                    m_vTilePos.y * SCALED_TILE_SIZE + SCALED_TILE_SIZE * 0.5f
                ),
                glm::vec2(s_rng.NextInt(-25, 25), s_rng.NextInt(-25, 25)),
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                4.0f,
                4.0f,
                s_pParticleTex
            );
        }
    }
    // Return if sibling is nullptr
    if(m_pSiblingPortalTile == nullptr) return;

    // Check if occupant exists
    wolf::GameObject* occupant = m_pLabyrinthManager->GetGameObject()->GetScene().GetObject(m_occupantID);
    if(occupant != nullptr && occupant->HasAll<wolf::Transform2D>())
    {
        // Get occupant position data
        glm::vec2 occupantPos = occupant->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        if (occupant->HasAll<PlayerController>())
        {
            occupantPos.y -= 12.0f;
        }
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

    // Check boss
    if (m_pBoss)
    {
        // But only if the controller is alive and not airborne
        auto* pController = m_pBoss->GetComponent<BossController>();
        if (pController && pController->IsAlive() && !pController->IsAirborne()) CheckTeleport(m_pBoss);
    }

    // Check all objects within the portal tile chunk
    for(wolf::GameObject* obj : GetChunk()->GetChildren())
    {
        // Don't tp harpies (they fly overhead)
        if (obj->HasAny<HarpyController>()) continue;
        CheckTeleport(obj);
    }

    // Check projectiles
    for (auto&& [_, velocity, attackDamage] : m_pLabyrinthManager->GetGameObject()->GetScene().Each<VelocityComponent, AttackDamageComponent>())
    {
        CheckTeleport(attackDamage.GetGameObject());
    }
    
}

void PortalTileManager::PortalTile::CheckTeleport(wolf::GameObject* p_obj)
{
    // Return if object is the same as the sprite obj
    if(p_obj->GetID() == m_pPortalTileSpriteObj->GetID()) return;
    
    // Return if object is occupant
    if(p_obj->GetID() == m_occupantID) return;

    // Return if object does not have VelocityComponent 
    if(!p_obj->HasAny<VelocityComponent>()) return;

    glm::ivec2 portalTilePos = GetTilePos();

    // Ensure transform is present
    wolf::Transform2D* objTransform = p_obj->GetComponent<wolf::Transform2D>();
    if (!objTransform) return;

    // Calculate tile position of object

    glm::vec2 objPos = objTransform->GetGlobalPosition();
    if (p_obj->HasAll<PlayerController>())
    {
        objPos.y -= 12.0f;
    }
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

    static wolf::RNG rng;
    wolf::Audio::Play("data/sounds/sfx_portal.wav", 0.6f, rng.NextInt(-8000, 0));
}