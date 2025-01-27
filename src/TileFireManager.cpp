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
    // Go through each fire column
    for(auto itr = m_mVerticalFireStrips.begin(); itr != m_mVerticalFireStrips.end(); itr++)
    {
        // Update each fire tile
        for (FireTile* fireTile : itr->second)
        {
            glm::vec2 worldpos = m_pLBMG->GetWorldPosition(fireTile->m_vTilePos);
            // GLShapesRenderer::GetInstance()->AddQuad({worldpos.x, worldpos.y, 1, 0, 0, 1}, 96, 96);
            if(fireTile->m_fLifespan <= 0.0f)
            {
                fireTile->m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(false);
            }
            else
            {
                fireTile->m_fLifespan -= p_delta;
                fireTile++;
            }
        }
    }

    // Check if player is in a tile
    glm::ivec2 playerTilePos = m_pLBMG->GetTilePosition(m_pPlayerObj->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
    int playerTileColumn = playerTilePos.x;

    auto itr = m_mVerticalFireStrips.find(playerTileColumn);
    if(itr != m_mVerticalFireStrips.end())
    {
        for(FireTile* firetile : m_mVerticalFireStrips.at(playerTileColumn))
        {
            if(firetile->m_vTilePos == playerTilePos)
            {
                m_pPlayerObj->GetComponent<StatusComponent>()->AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 5.0f);
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
    auto itr = m_mVerticalFireStrips.find(column);

    // If column not registered
    if(itr == m_mVerticalFireStrips.end())
    {
        // Create new column
        m_mVerticalFireStrips.insert({column, {}});
    }

    // If column has 1 or more fire tile
    if(m_mVerticalFireStrips.at(column).size() > 0)
    {
        for(auto fireTile : m_mVerticalFireStrips.at(column))
        {
            // Reset timer of matching fire tile
            if(fireTile->m_vTilePos.y == p_tile_pos.y)
            {
                fireTile->m_fLifespan = p_lifespan;
                fireTile->m_pFireObj->GetComponent<wolf::Sprite2D>()->SetVisibility(true);
                return;
            }
        }
    }

    // Create new fire tile if column is empty
    m_mVerticalFireStrips.at(column).emplace_back(new FireTile(m_pLBMG, p_tile_pos, p_lifespan));
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

    // AddFireTile(m_pLBMG->GetTilePosition(m_pLBMG->GetSpawnLocation() + glm::vec2(0.0f, 480.0f)) , 5.0f);
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
    // sprite->SetLayer(100);
    std::cout << "TFMG - FireObjID: " << m_pFireObj->GetID() << std::endl;
}

TileFireManager::FireTile::~FireTile()
{
    wolf::Scene* scene = &m_pFireObj->GetScene();
    scene->DeleteObject(m_pFireObj->GetID());
}