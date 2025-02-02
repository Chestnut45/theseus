#include "TileFireManager.h"
#include "GLShapesRenderer.h"

#include "components/StatusComponent.h"
#include "components/PlayerController.h"

TileFireManager* TileFireManager::s_pTFMG = nullptr;

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
    
    // Go through each fire column
    for(auto const& [column, columnVector] : m_mFireColumns)
    {
        // If column has any active fire tile
        if(m_vActiveFireColumnsTracker[column] > 0)
        {
            // Update each fire tile
            for (FireTile* fireTile : columnVector)
            {
                // If fire tile expired
                if(fireTile->m_fLifespan <= 0.0f)
                {
                    continue;
                }
                else
                {
                    // Update lifespan timer
                    fireTile->m_fLifespan -= p_delta;
                
                    // If fire tile will expire, update tracker & deactivate fire sprite
                    if(fireTile->m_fLifespan <= 0.0f)
                    {
                        m_vActiveFireColumnsTracker[column] -= 1;
                        fireTile->m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(false);
                    }
                }
            }
        }
    }

    // If player column has any active fire tile
    if(m_vActiveFireColumnsTracker[playerTileColumn] > 0)
    {
        for (FireTile* fireTile : m_mFireColumns[playerTileColumn])
        {
            // If matching tile position, and fire tile is still active, burn
            if(playerTilePos.y == fireTile->m_vTilePos.y && fireTile->m_fLifespan > 0.0f)
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

void TileFireManager::AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan)
{
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
                
                if(fireTile->m_fLifespan <= 0.0f)
                {
                    // Update tracker & turn on sprite
                    m_vActiveFireColumnsTracker[column] += 1;
                    fireTile->m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(true);
                }
                // Reset lifespan timer
                fireTile->m_fLifespan = p_lifespan;
                
                return;
            }
        }
    }

    // Create new fire tile if column is empty & update tracker
    m_mFireColumns[column].emplace_back(new FireTile(m_pLBMG, p_tile_pos, p_lifespan));
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

TileFireManager::FireTile::FireTile(LabyrinthManager* p_lbmg, glm::ivec2& p_tile_pos, float p_lifespan)
{
    m_vTilePos = p_tile_pos;
    m_fLifespan = p_lifespan;
    wolf::Scene* scene = &p_lbmg->GetGameObject()->GetScene();
    m_pFireObj = &scene->CreateObject2D();
    m_pFireObj->GetComponent<wolf::Transform2D>()->SetPosition(p_lbmg->GetWorldPosition(m_vTilePos));
    m_pFireObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f, 3.0f));
    wolf::Sprite2D* sprite = &m_pFireObj->AddComponent<wolf::Sprite2D>("data/textures/Fireball.png");
}

TileFireManager::FireTile::~FireTile()
{
    wolf::Scene* scene = &m_pFireObj->GetScene();
    scene->DeleteObject(m_pFireObj->GetID());
}