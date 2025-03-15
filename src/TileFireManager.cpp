//-----------------------------------------------------------------------------
// File: TileFireManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages tile-based fire.
//-----------------------------------------------------------------------------
#include "TileFireManager.h"
#include "GLShapesRenderer.h"

#include "components/StatusComponent.h"
#include "components/PlayerController.h"

TileFireManager* TileFireManager::s_pTFMG = nullptr;
wolf::RNG TileFireManager::FireTile::s_rng;

void TileFireManager::CreateInstance(LabyrinthManager* p_lbmg)
{
    if(s_pTFMG == nullptr)
    {
        s_pTFMG = new TileFireManager(p_lbmg);
    }
}

void TileFireManager::DestroyInstance()
{
    if(s_pTFMG != nullptr)
    {
        delete s_pTFMG;
        s_pTFMG = nullptr;
    }
}

TileFireManager* TileFireManager::GetInstance()
{
    return s_pTFMG;
}

void TileFireManager::Update(float p_delta)
{

    glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(m_pPlayerObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    int playerTileColumn = playerTilePos.x;
    // std::cout << "playerpos - x: " << playerTilePos.x << ", y: " << playerTilePos.y << std::endl;

    if(wolf::Input::IsKeyJustDown(GLFW_KEY_F))
    {
        AddFireTile(playerTilePos);   
    }
    
    // Go through each fire column
    for(auto const& [column, columnTiles] : m_mFireColumns)
    {
        for (int i = 0; i < columnTiles.size(); i++)
        {
            FireTile* fireTile = columnTiles.at(i);
            if(fireTile != nullptr)
            {
                // Update each fire tile
                fireTile->Update(p_delta);
            }
        }
    }

    // Player is not rolling
    if(
        m_pPlayerObj->GetComponent<PlayerController>()->GetPlayerAction() != PlayerController::PlayerAction::ROLLING &&
        m_mFireColumns.find(playerTilePos.x) != m_mFireColumns.end()
    )
    {
        for (FireTile* fireTile : m_mFireColumns[playerTileColumn])
        {
            // If fire tile is still active & matching tile position, burn
            if(fireTile->m_currentBurnState == FireTile::BurnState::BURNING && playerTilePos.y == fireTile->m_vTilePos.y)
            {
                m_pPlayerObj->GetComponent<StatusComponent>()->AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 5.0f);
                break;
            }
        }
    }
}

void TileFireManager::Render()
{
}

void TileFireManager::AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan, float p_cooldown, bool p_reset_lifespan)
{
    int tileID = m_pLBMG->GetTile(p_tile_pos.x, p_tile_pos.y);
    if(tileID == -2) return;
    if(IsWallTile(tileID))return;
    float lifespan = p_lifespan < 0.0f ? m_fStockLifespan : p_lifespan;
    float burntCooldown = p_cooldown < 0.0f ? m_fStockBurntCooldown : p_cooldown;
    int column = p_tile_pos.x;

    // Check if column is registered
    auto itr = m_mFireColumns.find(column);

    // If column not registered
    if(itr == m_mFireColumns.end())
    {
        // Create new column & new fire tile
        m_mFireColumns.insert({column, {new FireTile(m_pLBMG, p_tile_pos, lifespan, burntCooldown)}});
        return;
    }

    // If column already exists
    else{
        for(auto fireTile : m_mFireColumns[column])
        {
            // If matching fire tile

            if(fireTile->m_vTilePos.y == p_tile_pos.y)
            {
                if(p_reset_lifespan == false) return;
                if(fireTile->m_currentBurnState == FireTile::BurnState::BURNT) return;

                fireTile->Reset(lifespan, burntCooldown);
                return;
            }
        }

        // If no matching tile, create new fire tile & update tracker
        m_mFireColumns[column].push_back(new FireTile(m_pLBMG, p_tile_pos, lifespan, burntCooldown));
    }

    return;
}

TileFireManager::TileFireManager(LabyrinthManager* p_lbmg)
{
    m_pLBMG = p_lbmg;
    m_pScene = &m_pLBMG->GetGameObject()->GetScene();
    for(auto&& [_, playerController] : m_pScene->Each<PlayerController>())
    {
        m_pPlayerObj = playerController.GetGameObject();
        break;
    }
}

TileFireManager::~TileFireManager()
{
    for (auto itr = m_mFireColumns.begin(); itr != m_mFireColumns.end();) {
        for(auto& fireTile : itr->second)
        {
            delete fireTile;
            fireTile = nullptr;
        }
        itr->second.clear();
        itr = m_mFireColumns.erase(itr);
    }
    m_pLBMG = nullptr;
    m_pScene = nullptr;

}

bool TileFireManager::IsWallTile(int p_tile_id)
{
    return (p_tile_id >= Tile::WallBottomLeft) && (p_tile_id <= Tile::WallTop);
}

//------------------//
//                  //
//  Struct Methods  //
//                  //
//------------------//

TileFireManager::FireTile::FireTile(LabyrinthManager* p_lbmg, glm::ivec2& p_tile_pos, float p_lifespan, float p_cooldown)
{
    // Set member variables
    m_currentBurnState = BurnState::BURNING;
    m_fLifespan = p_lifespan;
    m_fBurntCooldown = p_cooldown;

    m_vTilePos = p_tile_pos;
    glm::ivec4 tilePosLRTB = glm::ivec4(p_tile_pos.x - 1, p_tile_pos.x + 1, p_tile_pos.y + 1, p_tile_pos.y - 1);
    
    int tileLID = p_lbmg->GetTile(tilePosLRTB.x, m_vTilePos.y);    // Left
    int tileRID = p_lbmg->GetTile(tilePosLRTB.y, m_vTilePos.y);    // Right
    int tileTID = p_lbmg->GetTile(m_vTilePos.x, tilePosLRTB.z);    // Top
    int tileBID = p_lbmg->GetTile(m_vTilePos.x, tilePosLRTB.w);    // Bottom
 
    tilePosLRTB = glm::ivec4(
        tileLID > -2 ? tilePosLRTB.x : tileLID, 
        tileRID > -2 ? tilePosLRTB.y : tileRID,
        tileTID > -2 ? tilePosLRTB.z : tileTID,
        tileBID > -2 ? tilePosLRTB.w : tileBID
    );

    const int boundsCheck = -2;

    m_aNeighbourPos = {
        glm::ivec2(tilePosLRTB.x, m_vTilePos.y),    // Left Centre
        glm::ivec2(tilePosLRTB.y, m_vTilePos.y),    // Right Centre
        glm::ivec2(m_vTilePos.x, tilePosLRTB.z),    // Top Centre
        glm::ivec2(m_vTilePos.x, tilePosLRTB.w),    // Bottom Centre
        glm::ivec2(tilePosLRTB.x, tilePosLRTB.z),   // Left Top
        glm::ivec2(tilePosLRTB.x, tilePosLRTB.w),   // Left Bottom
        glm::ivec2(tilePosLRTB.y, tilePosLRTB.z),   // Right Top
        glm::ivec2(tilePosLRTB.y, tilePosLRTB.w),   // Right Bottom
    };

    m_aBounds = {
        tilePosLRTB.x > boundsCheck,                                // Left Centre
        tilePosLRTB.y > boundsCheck,                                // Right Centre
        tilePosLRTB.z > boundsCheck,                                // Top Centre
        tilePosLRTB.w > boundsCheck,                                // Bottom Centre
        tilePosLRTB.x > boundsCheck && tilePosLRTB.z > boundsCheck, // Left Top
        tilePosLRTB.x > boundsCheck && tilePosLRTB.w > boundsCheck, // Left Bottom
        tilePosLRTB.y > boundsCheck && tilePosLRTB.z > boundsCheck, // Right Top
        tilePosLRTB.y > boundsCheck && tilePosLRTB.w > boundsCheck, // Right Bottom
    };

    wolf::Scene* scene = &p_lbmg->GetGameObject()->GetScene();

    // Create fireObj
    m_pFireObj = &scene->CreateObject2D();
    m_pFireObj->GetComponent<wolf::Transform2D>()->SetPosition(p_lbmg->GetWorldPosition(m_vTilePos) + glm::vec2(LabyrinthManager::TILE_SIZE * 1.5f));
    m_pFireObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
    wolf::Sprite2D* fireSprite = &m_pFireObj->AddComponent<wolf::Sprite2D>("data/textures/Fireball.png");
    fireSprite->SetOriginToCenterOfTexture();
    fireSprite->SetVisibility(true);

    // create burntTileObj

    m_pBurntTileObj = &scene->CreateObject2D();
    m_pBurntTileObj->GetComponent<wolf::Transform2D>()->SetPosition(p_lbmg->GetWorldPosition(m_vTilePos) + glm::vec2(LabyrinthManager::TILE_SIZE * 1.5f));
    m_pBurntTileObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(LabyrinthManager::SCALE));
    wolf::Sprite2D* scorchSprite = &m_pBurntTileObj->AddComponent<wolf::Sprite2D>("data/textures/scorch.png");
    scorchSprite->SetOriginToCenterOfTexture();
    scorchSprite->SetVisibility(false);
    
    s_rng.NextFloat(0.0f, 1.0f);
}

TileFireManager::FireTile::~FireTile()
{
    wolf::Scene* scene = &m_pFireObj->GetScene();
    scene->DeleteObject(m_pFireObj->GetID());
    scene->DeleteObject(m_pBurntTileObj->GetID());
}

void TileFireManager::FireTile::Update(float p_delta)
{
    switch(m_currentBurnState)
    {
        case FireTile::BurnState::BURNING:
        {
            HandleBurningState(p_delta);
            break;
        }
        case FireTile::BurnState::BURNT:
        {
    
            HandleBurntState(p_delta);
            break;
        }
        case FireTile::BurnState::UNBURNT:
        {
            HandleUnburntState(p_delta);
            break;
        }
        default:
            break;
    }
}
void TileFireManager::FireTile::Reset(float p_lifespan, float p_cooldown)
{
    // If tile is in BURNT state
    if(m_currentBurnState == FireTile::BurnState::BURNT) return;

    // If tile is in UNBURNT state
    if(m_currentBurnState == FireTile::BurnState::UNBURNT)
    {
        // Set sprites visibility
        m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(true);
        m_pBurntTileObj->GetComponent<wolf::Sprite2D>()->SetVisibility(false);
    }
    // Reset lifespan & burnt cooldown timer
    m_currentBurnState = BurnState::BURNING;
    m_fLifespan = p_lifespan;
    m_fBurntCooldown = p_cooldown;
    return;
}

void TileFireManager::FireTile::AttemptPropagation()
{

    for (int i = 0; i < 8; ++i) {
        if (m_aBounds[i]) {
            if (!s_pTFMG->IsWallTile(s_pTFMG->m_pLBMG->GetTile(m_aNeighbourPos[i].x, m_aNeighbourPos[i].y))) {
                if (s_rng.NextFloat(0.0f, 1.0f) <= m_fSpreadChance) {
                    s_pTFMG->AddFireTile(m_aNeighbourPos[i], -1, -1, false);
                }
            }
        }
    }
}

void TileFireManager::FireTile::HandleBurningState(float p_delta)
{
    // If fire lifespan expired
    if(m_fLifespan <= 0.0f)
    {
        m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(false);
        m_pBurntTileObj->GetComponent<wolf::Sprite2D>()->SetVisibility(true);
        m_currentBurnState = BurnState::BURNT;
        return;
    }
    else
    {
        // Update lifespan timer
        m_fLifespan -= p_delta;

        // Update spread delay timer
        m_fSpreadDelayTimer -= p_delta;

        // If spread delay expired
        if(m_fSpreadDelayTimer <= 0.0f)
        {
            // Reset timer
            m_fSpreadDelayTimer = m_fSpreadDelay;
            
            // Attempt to propagate fire
            AttemptPropagation();
        }
    }
}

void TileFireManager::FireTile::HandleBurntState(float p_delta)
{
    // If burnt cooldown expired
    if(m_fBurntCooldown <= 0.0f)
    {

        m_pBurntTileObj->GetComponent<wolf::Sprite2D>()->SetVisibility(false);
        m_currentBurnState = BurnState::UNBURNT;
        return;
    }
    else
    {
        // Update lifespan timer
        m_fBurntCooldown -= p_delta;
    }
}

void TileFireManager::FireTile::HandleUnburntState(float p_delta)
{
    return;
}