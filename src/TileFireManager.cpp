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
    for(auto const& [column, columnVector] : m_mFireColumns)
    {
        // If column has any active fire tile
        if(m_vActiveFireColumnsTracker[column] > 0)
        {

        }
        for (FireTile* fireTile : columnVector)
        {

            if
            (
                (fireTile->m_fLifespan > 0.0f) && 
                (fireTile->m_fLifespan - p_delta <= 0.0f)
            )
            {
                s_pTFMG->m_vActiveFireColumnsTracker[column] -= 1;
            }
            // Update each fire tile
            fireTile->Update(p_delta);
        }
    }

    // If player column has any active fire tile && player is not rolling
    if(
        m_vActiveFireColumnsTracker[playerTileColumn] > 0                                                               &&
        m_pPlayerObj->GetComponent<PlayerController>()->GetPlayerAction() != PlayerController::PlayerAction::ROLLING
    )
    {
        for (FireTile* fireTile : m_mFireColumns[playerTileColumn])
        {
            // If matching tile position, and fire tile is still active, burn
            if(playerTilePos.y == fireTile->m_vTilePos.y && fireTile->m_currentBurnState == FireTile::BurnState::BURNING)
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

void TileFireManager::AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan, float p_cooldown)
{
    float lifespan = p_lifespan < 0.0f ? m_fStockLifespan : p_lifespan;
    float burntCooldown = p_cooldown < 0.0f ? m_fStockBurntCooldown : p_cooldown;
    int column = p_tile_pos.x;

    // Check if column is registered
    auto itr = m_mFireColumns.find(column);

    // If column not registered
    if(itr == m_mFireColumns.end())
    {
        // Create new column
        m_mFireColumns.insert({column, {}});
    }

    // If column has 1 or more fire tile
    
    if(m_mFireColumns[column].size() > 0)
    {
        for(auto fireTile : m_mFireColumns[column])
        {
            // If matching fire tile
            if(fireTile->m_vTilePos.y == p_tile_pos.y)
            {
                if(fireTile->m_currentBurnState == FireTile::BurnState::UNBURNT)
                {
                    // Update tracker & turn on sprite
                    m_vActiveFireColumnsTracker[column] += 1;
                }
                fireTile->Reset(lifespan, burntCooldown);
            }
        }
    }

    // Create new fire tile if column is empty & update tracker
    m_mFireColumns[column].emplace_back(new FireTile(m_pLBMG, p_tile_pos, lifespan, burntCooldown));
    m_vActiveFireColumnsTracker[column] += 1;

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

    m_vActiveFireColumnsTracker.resize(LabyrinthManager::MAX_LABYRINTH_DIM);
    for(int i = 0; i < LabyrinthManager::MAX_LABYRINTH_DIM; i++)
    {
        m_vActiveFireColumnsTracker[i] = 0;
    }
}

TileFireManager::~TileFireManager()
{
    m_pLBMG = nullptr;
    m_pScene = nullptr;
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
    m_vTilePos = p_tile_pos;
    m_fLifespan = p_lifespan;
    m_fBurntCooldown = p_cooldown;

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
    // If tile is in UNBURNT state
    if(m_currentBurnState == FireTile::BurnState::BURNT) return;

    // If tile is in UNBURNT state
    if(m_currentBurnState == FireTile::BurnState::UNBURNT)
    {
        // Update tracker & turn on sprite
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
    float rng = s_rng.NextFloat(0.0f, 1.0f);
    if(rng > m_fSpreadChance) return;

    // if(s_pTFMG->m_mFireColumns)
    // {}
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