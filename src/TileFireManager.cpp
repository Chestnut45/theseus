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

    // If player is not rolling & on a registered column
    if(
        m_pPlayerObj->GetComponent<PlayerController>()->GetPlayerAction() != PlayerController::PlayerAction::ROLLING &&
        m_mFireColumns.find(playerTilePos.x) != m_mFireColumns.end()
    )
    {
        // Interate through every tile in that column
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

void TileFireManager::AddFireTile(glm::ivec2 p_tile_pos, float p_lifespan, float p_cooldown, bool p_reset_burning_lifespan)
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
                // Return if tile is burnt
                if(fireTile->m_currentBurnState == FireTile::BurnState::BURNT) return;
                // Return if tile is burning but lifespan reset flag is false
                if(fireTile->m_currentBurnState == FireTile::BurnState::BURNING && p_reset_burning_lifespan == false) return;

                // Reset & Return
                fireTile->Reset(lifespan, burntCooldown);
                return;
            }
        }

        // If no matching tile, create new fire tile
        m_mFireColumns[column].push_back(new FireTile(m_pLBMG, p_tile_pos, lifespan, burntCooldown));
    }

    return;
}

void TileFireManager::SetPropagationActiveness(bool p_propagation)
{
    m_bIsPropagationEnabled = p_propagation;
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

    // Get cardinal X & Y positions around tile
    glm::ivec4 tilePosLRTB = glm::ivec4(
        p_tile_pos.x - 1, 
        p_tile_pos.x + 1, 
        p_tile_pos.y + 1, 
        p_tile_pos.y - 1
    );
    
    // Out of bounds checkers through ID
    int tileLID = p_lbmg->GetTile(tilePosLRTB.x, m_vTilePos.y);    // Left
    int tileRID = p_lbmg->GetTile(tilePosLRTB.y, m_vTilePos.y);    // Right
    int tileTID = p_lbmg->GetTile(m_vTilePos.x, tilePosLRTB.z);    // Top
    int tileBID = p_lbmg->GetTile(m_vTilePos.x, tilePosLRTB.w);    // Bottom
 

    const int boundsCheck = -2;

    // Store all neighbour positions
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
    
    // Set to -2 if out of bounds
    tilePosLRTB = glm::ivec4(
        tileLID > -2 ? tilePosLRTB.x : tileLID, 
        tileRID > -2 ? tilePosLRTB.y : tileRID,
        tileTID > -2 ? tilePosLRTB.z : tileTID,
        tileBID > -2 ? tilePosLRTB.w : tileBID
    );

    // Store neighbour bounds check
    m_aInBounds = {
        tilePosLRTB.x > boundsCheck,                                // West
        tilePosLRTB.y > boundsCheck,                                // East
        tilePosLRTB.z > boundsCheck,                                // North
        tilePosLRTB.w > boundsCheck,                                // South
        tilePosLRTB.x > boundsCheck && tilePosLRTB.z > boundsCheck, // NorthWest
        tilePosLRTB.x > boundsCheck && tilePosLRTB.w > boundsCheck, // SouthWest
        tilePosLRTB.y > boundsCheck && tilePosLRTB.z > boundsCheck, // NorthEast
        tilePosLRTB.y > boundsCheck && tilePosLRTB.w > boundsCheck, // SouthWest
    };

    m_aNotWalls = {
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::WEST].x, m_aNeighbourPos[Direction::WEST].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::EAST].x, m_aNeighbourPos[Direction::EAST].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::NORTH].x, m_aNeighbourPos[Direction::NORTH].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::SOUTH].x, m_aNeighbourPos[Direction::SOUTH].y)),   
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::NORTHWEST].x, m_aNeighbourPos[Direction::NORTHWEST].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::SOUTHWEST].x, m_aNeighbourPos[Direction::SOUTHWEST].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::NORTHEAST].x, m_aNeighbourPos[Direction::NORTHEAST].y)),
        !IsWallTile(p_lbmg->GetTile(m_aNeighbourPos[Direction::SOUTHEAST].x, m_aNeighbourPos[Direction::SOUTHEAST].y))
    };

    m_aCardinalNeighbourPairIndices = {
        glm::ivec2(Direction::WEST, Direction::NORTH),
        glm::ivec2(Direction::WEST, Direction::SOUTH),
        glm::ivec2(Direction::EAST, Direction::NORTH),
        glm::ivec2(Direction::EAST, Direction::SOUTH),
    };

    // Each weight is added by 1 and multiplied by the spread chance
    float weight = 0.25f; 

    // Initialise every weights as 0.0f
    std::fill(m_aNeighbourWeights.begin(), m_aNeighbourWeights.end(), 0.0f);
    
    // Cardinal tiles
    for(int i = Direction::WEST; i <= Direction::SOUTH; i++)
    {
        // If the cardinal tile is out of bounds or is wall tile
        if(m_aInBounds[i] == false || !m_aNotWalls[i])
        {         
            Direction dir = (Direction)i;
            
            // Switch directions
            // For each ordinal neighbour, that is within bounds and not a wall, of that cardinal tile, add weights to that tile
            switch(dir)
            {
                case Direction::WEST:
                {
                    glm::ivec2 n = m_aNeighbourPos[Direction::NORTH];
                    if(m_aInBounds[Direction::NORTH] && m_aNotWalls[Direction::NORTH])
                    {
                        m_aNeighbourWeights[Direction::NORTH] += weight;
                    }

                    glm::ivec2 s = m_aNeighbourPos[Direction::SOUTH];
                    if(m_aInBounds[Direction::SOUTH] && m_aNotWalls[Direction::SOUTH])
                    {
                        m_aNeighbourWeights[Direction::SOUTH] += weight;
                    }

                    break;
                }
                case Direction::EAST:
                {         
                    glm::ivec2 n = m_aNeighbourPos[Direction::NORTH];
                    if(m_aInBounds[Direction::NORTH] && m_aNotWalls[Direction::NORTH])
                    {
                        m_aNeighbourWeights[Direction::NORTH] += weight;
                    }

                    glm::ivec2 s = m_aNeighbourPos[Direction::SOUTH];
                    if(m_aInBounds[Direction::SOUTH] && m_aNotWalls[Direction::SOUTH])
                    {
                        m_aNeighbourWeights[Direction::SOUTH] += weight;
                    }
                    break;
                }
                case Direction::NORTH:
                {
                    glm::ivec2 w = m_aNeighbourPos[Direction::WEST];
                    if(m_aInBounds[Direction::WEST] && m_aNotWalls[Direction::WEST])
                    {
                        m_aNeighbourWeights[Direction::WEST] += weight;
                    }

                    glm::ivec2 e = m_aNeighbourPos[Direction::EAST];
                    if(m_aInBounds[Direction::EAST] && m_aNotWalls[Direction::EAST])
                    {
                        m_aNeighbourWeights[Direction::EAST] += weight;
                    }
                    break;
                }
                case Direction::SOUTH:
                {
                    glm::ivec2 w = m_aNeighbourPos[Direction::WEST];
                    if(m_aInBounds[Direction::WEST] && m_aNotWalls[Direction::WEST])
                    {
                        m_aNeighbourWeights[Direction::WEST] += weight;
                    }

                    glm::ivec2 e = m_aNeighbourPos[Direction::EAST];
                    if(m_aInBounds[Direction::EAST] && m_aNotWalls[Direction::EAST])
                    {
                        m_aNeighbourWeights[Direction::EAST] += weight;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    // Ordinal tiles
    for(int i = Direction::NORTHWEST; i <= Direction::SOUTHEAST; i++)
    {
        // If the ordinal tile is out of bounds or is wall tile
        if(m_aInBounds[i] == false || !m_aNotWalls[i])
        {           
            Direction dir = (Direction)i;
            
            // Switch directions
            // For each cardinal neighbour, that is within bounds and not a wall, of that ordinal tile, add weights to that tile
            switch(dir)
            {
                case Direction::NORTHWEST: 
                {
                    glm::ivec2 n = m_aNeighbourPos[Direction::NORTH];
                    if(m_aInBounds[Direction::NORTH] && m_aNotWalls[Direction::NORTH])
                    {
                        m_aNeighbourWeights[Direction::NORTH] += weight;
                    }
                    
                    glm::ivec2 w = m_aNeighbourPos[Direction::WEST];
                    if(m_aInBounds[Direction::WEST] && m_aNotWalls[Direction::WEST])
                    {
                        m_aNeighbourWeights[Direction::WEST] += weight;
                    }
                    break;
                }
                
                case Direction::SOUTHWEST: 
                {
                    glm::ivec2 s = m_aNeighbourPos[Direction::SOUTH];
                    if(m_aInBounds[Direction::SOUTH] && m_aNotWalls[Direction::SOUTH])
                    {
                        m_aNeighbourWeights[Direction::SOUTH] += weight;
                    }
                    
                    glm::ivec2 w = m_aNeighbourPos[Direction::WEST];
                    if(m_aInBounds[Direction::WEST] && m_aNotWalls[Direction::WEST])
                    {
                        m_aNeighbourWeights[Direction::WEST] += weight;
                    }
                    break;
                }

                case Direction::NORTHEAST:
                {
                    glm::ivec2 n = m_aNeighbourPos[Direction::NORTH];
                    if(m_aInBounds[Direction::NORTH] && m_aNotWalls[Direction::NORTH])
                    {
                        m_aNeighbourWeights[Direction::NORTH] += weight;
                    }
                    
                    glm::ivec2 e = m_aNeighbourPos[Direction::EAST];
                    if(m_aInBounds[Direction::EAST] && m_aNotWalls[Direction::EAST])
                    {
                        m_aNeighbourWeights[Direction::EAST] += weight;
                    }
                    break;
                }

                case Direction::SOUTHEAST:
                {
                    glm::ivec2 s = m_aNeighbourPos[Direction::SOUTH];
                    if(m_aInBounds[Direction::SOUTH] && m_aNotWalls[Direction::SOUTH])
                    {
                        m_aNeighbourWeights[Direction::SOUTH] += weight;
                    }
                    
                    glm::ivec2 e = m_aNeighbourPos[Direction::EAST];
                    if(m_aInBounds[Direction::EAST] && m_aNotWalls[Direction::EAST])
                    {
                        m_aNeighbourWeights[Direction::EAST] += weight;
                    }
                    break;
                }

                default:
                {
                    break;
                }
            }
        }
    }

    // for(int i = 0; i < 8; i++)
    // {
    //     std::cout << i << ": " << m_aNeighbourWeights[i] << std::endl;
    // }
    // printf("-------\n");

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
    
    // Prevent first RNG roll being constant
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
    // Return if tile is in BURNT state
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
    // Cardinal propagation
    for (int i = 0; i < 4; i++) 
    {
        // If the cardinal tile is within bounds
        if (m_aInBounds[i]) 
        {
            // If rng check passes       
            if (s_rng.NextFloat(0.0f, 1.0f) <= m_fCardinalSpreadChance * (1 + m_aNeighbourWeights[i])) 
            {
                // If the cardinal tile is not a wall
                if (m_aNotWalls[i]) 
                {
                    // Propagate
                    s_pTFMG->AddFireTile(m_aNeighbourPos[i], -1, -1, false);
                    
                }
            }
        }
    }

    // Ordinal propagation
    for (int i = 4; i < 8; i++) 
    {
        // If the ordinal tile is within bounds
        if (m_aInBounds[i]) 
        {

            // If rng check passes
            if (s_rng.NextFloat(0.0f, 1.0f) <= m_fOrdinalSpreadChance  * (1 + m_aNeighbourWeights[i])) 
            {
                glm::ivec2 cnpIndices = m_aCardinalNeighbourPairIndices[i-4];

                // If the ordinal tile is not a wall && neither of the cardinal neighbour tiles is a wall 
                if (
                    !m_aNotWalls[i]             &&
                    !m_aNotWalls[cnpIndices.x]  &&
                    !m_aNotWalls[cnpIndices.y]
                ) 
                {
                    // Propagate
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

        // If propagation flag set to true
        if(s_pTFMG->m_bIsPropagationEnabled == true)
        {
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