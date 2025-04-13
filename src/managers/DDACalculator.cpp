//-----------------------------------------------------------------------------
// File: DDACalculator.cpp
// Original Author: Nguyễn Minh Nhật
// Contains methods related to line-based grid traversal
//-----------------------------------------------------------------------------

#include "DDACalculator.h"

DDACalculator* DDACalculator::s_pDDAC = nullptr;

//------------------//
//                  //
//  Public Methods  //
//                  //
//------------------//

void DDACalculator::CreateInstance(wolf::Scene* p_scene)
{
    if(s_pDDAC != nullptr)
    {
        return;
    }
    s_pDDAC = new DDACalculator(p_scene);
}

void DDACalculator::DestroyInstance()
{
    if(s_pDDAC == nullptr)
    {
        return;
    }
    delete s_pDDAC;
    s_pDDAC = nullptr;
}

DDACalculator* DDACalculator::GetInstance()
{
    return s_pDDAC;
}

// Returns the endpoint of the line between the source and the destination
// If the line is blocked by a wall, return the intersection point between the line & the wall
glm::vec2 DDACalculator::GetEndpoint(glm::vec2 p_src_pos, glm::vec2 p_dst_pos)
{
    // If labyrinth manager is not nullptr
    if(this->m_pLBMG != nullptr)
    {
        glm::ivec2 srcTilePos = this->m_pLBMG->GetTilePosition(p_src_pos);
        int srcTileID = this->m_pLBMG->GetTile(srcTilePos.x, srcTilePos.y);

        glm::ivec2 dstTilePos = this->m_pLBMG->GetTilePosition(p_dst_pos);
        int dstTileID = this->m_pLBMG->GetTile(dstTilePos.x, dstTilePos.y);

        // If source is on a wall tile, return source
        if
        (
            srcTilePos != glm::ivec2(-1, -1)    && 
            this->IsWallTile(srcTileID) == true
        )
        {
            return p_src_pos;
        }
        
        // If source & destination are on the same tile, return destination
        if
        (
            srcTilePos != glm::ivec2(-1, -1)    &&
            srcTilePos == dstTilePos
        )
        {
            return p_dst_pos;
        }

        const int tileSize = (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        // calculate vector from source to destination & related data
        glm::vec2 line = p_dst_pos - p_src_pos;
        glm::vec2 normalisedLine = glm::normalize(line);
        float distance = glm::length(line);

        glm::ivec2 currentTilePos = srcTilePos;
        int currentTileID = srcTileID;
        
        // Length of ray when stepping along an axis
        glm::vec2 rayLength = glm::vec2(0.0f, 0.0f);

        
        // incrementor to calculate current tile of ray
        glm::ivec2 tileStep = glm::vec2(0, 0);

        
        // Length of step when stepping along an axis
        glm::vec2 rayStep = glm::vec2(
            sqrt(1 + (normalisedLine.y / normalisedLine.x) * (normalisedLine.y / normalisedLine.x)),
            sqrt(1 + (normalisedLine.x / normalisedLine.y) * (normalisedLine.x / normalisedLine.y))
        );

        // Initialise length of rays & tile incrementors based on position of source
        // Line is going right
        if(line.x > 0.0f)
        {
            tileStep.x = 1;
            rayLength.x = abs(this->GetTileWorldPos(glm::ivec2(srcTilePos.x + 1, srcTilePos.y)).x - p_src_pos.x) * rayStep.x;
        }
        // Line is going left
        else
        {
            tileStep.x = -1;
            rayLength.x = abs(p_src_pos.x - this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y)).x) * rayStep.x;
        }
        // Line is going up
        if(line.y > 0.0f)
        {
            tileStep.y = 1;
            rayLength.y = abs(this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y + 1)).y - p_src_pos.y) * rayStep.y;
        }
        // Line is going down
        else
        {
            tileStep.y = -1;
            rayLength.y = abs(p_src_pos.y - this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y)).y) * rayStep.y;
        }
        
        rayStep *= tileSize;
        
        // Iterate until target tile is reached
        bool isTargetSpotted = true;
        bool isIterating = true;
        float distanceCheck = 0.0f;

        // Iterate until the distance iterated is greater than the distance between source & destination
        while(distanceCheck < distance)
        {
            // Increment ray x if ray x is shorter than ray y
            if(rayLength.x < rayLength.y)
            {
                currentTilePos.x += tileStep.x;
                distanceCheck = rayLength.x;
                rayLength.x += rayStep.x;
            }
            // Increment ray y if ray y is shorter than ray x
            else
            {
                currentTilePos.y += tileStep.y;
                distanceCheck = rayLength.y;
                rayLength.y += rayStep.y;
            }

            currentTileID = this->m_pLBMG->GetTile(currentTilePos.x, currentTilePos.y);
            
            // If blocked by a wall, return enpoint
            if(this->IsWallTile(currentTileID))
            {
                if(distanceCheck < distance)
                {
                    glm::vec2 endpoint = normalisedLine * distanceCheck + p_src_pos;
                    return endpoint;
                }
                break;
            }
        }
    }

    // Return destination position by default
    return p_dst_pos;
}


std::vector<glm::ivec2> DDACalculator::GetTraversedTiles(glm::vec2 p_src_pos, glm::vec2 p_dst_pos, bool p_is_blocked)
{
    std::vector<glm::ivec2> tiles;

    // If labyrinth manager is not nullptr
    if(this->m_pLBMG != nullptr)
    {   
        // Calculate data for source
        glm::ivec2 srcTilePos = this->m_pLBMG->GetTilePosition(p_src_pos);
        int srcTileID = this->m_pLBMG->GetTile(srcTilePos.x, srcTilePos.y);
        
        // Calculate data for destination
        glm::ivec2 dstTilePos = this->m_pLBMG->GetTilePosition(p_dst_pos);
        int dstTileID = this->m_pLBMG->GetTile(dstTilePos.x, dstTilePos.y);

        tiles.push_back(srcTilePos);

        // If source is on a wall tile, return only the source tile
        if
        (
            srcTilePos != glm::ivec2(-1, -1)    && 
            this->IsWallTile(srcTileID) == true
        )
        {
            return tiles;
        }
        
        // If source & destination are on the same tile, return only the source tile
        if
        (
            srcTilePos != glm::ivec2(-1, -1)    &&
            srcTilePos == dstTilePos
        )
        {
            return tiles;
        }

        const int tileSize = (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        // calculate vector from source to destination & related data
        glm::vec2 line = p_dst_pos - p_src_pos;
        glm::vec2 normalisedLine = glm::normalize(line);
        float distance = glm::length(line);

        glm::ivec2 currentTilePos = srcTilePos;
        int currentTileID = srcTileID;
        
        // Length of ray when stepping along an axis
        glm::vec2 rayLength = glm::vec2(0.0f, 0.0f);

        
        // incrementor to calculate current tile of ray
        glm::ivec2 tileStep = glm::vec2(0, 0);

        
        // Length of step when stepping along an axis
        glm::vec2 rayStep = glm::vec2(
            sqrt(1 + (normalisedLine.y / normalisedLine.x) * (normalisedLine.y / normalisedLine.x)),
            sqrt(1 + (normalisedLine.x / normalisedLine.y) * (normalisedLine.x / normalisedLine.y))
        );

        // Initialise length of rays & tile incrementors based on position of source
        // Line is going right
        if(line.x > 0.0f)
        {
            tileStep.x = 1;
            rayLength.x = abs(this->GetTileWorldPos(glm::ivec2(srcTilePos.x + 1, srcTilePos.y)).x - p_src_pos.x) * rayStep.x;
        }
        // Line is going left
        else
        {
            tileStep.x = -1;
            rayLength.x = abs(p_src_pos.x - this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y)).x) * rayStep.x;
        }
        // Line is going up
        if(line.y > 0.0f)
        {
            tileStep.y = 1;
            rayLength.y = abs(this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y + 1)).y - p_src_pos.y) * rayStep.y;
        }
        // Line is going down
        else
        {
            tileStep.y = -1;
            rayLength.y = abs(p_src_pos.y - this->GetTileWorldPos(glm::ivec2(srcTilePos.x, srcTilePos.y)).y) * rayStep.y;
        }
        
        rayStep *= tileSize;
        
        // Iterate until target tile is reached
        bool isTargetSpotted = true;
        bool isIterating = true;
        float distanceCheck = 0.0f;

        while(distanceCheck < distance)
        {
            // Increment ray x if ray x is shorter than ray y
            if(rayLength.x < rayLength.y)
            {
                currentTilePos.x += tileStep.x;
                distanceCheck = rayLength.x;
                rayLength.x += rayStep.x;
            }
            // Increment ray y if ray y is shorter than ray x
            else
            {
                currentTilePos.y += tileStep.y;
                distanceCheck = rayLength.y;
                rayLength.y += rayStep.y;
            }

            currentTileID = this->m_pLBMG->GetTile(currentTilePos.x, currentTilePos.y);
            
            // If blocked by a wall, return the current set of tiles
            if(p_is_blocked && this->IsWallTile(currentTileID))
            {
                if(distanceCheck < distance)
                {
                    return tiles;
                }
                break;
            }
            // If not, add the tile
            else
            {
                tiles.push_back(currentTilePos);
            }

            // If destination tile is reached, break
            if(currentTilePos == dstTilePos)
            {
                break;
            }
        }
    }
    // Return the tiles traversed
    return tiles;
}

//--------------------//
//                    //
//  Priivate Methods  //
//                    //
//--------------------//

DDACalculator::DDACalculator(wolf::Scene* p_scene)
{
    this->m_pScene = p_scene;
    for (auto&& [_, lbmg] : this->m_pScene->Each<LabyrinthManager>())
    {
        this->m_pLBMG = &lbmg;
        break;
    }
}

DDACalculator::~DDACalculator()
{

}

bool DDACalculator::IsWallTile(int p_tile_id)
{
    return (p_tile_id >= Tile::WallBottomLeft) && (p_tile_id <= Tile::WallTop);
}

glm::vec2 DDACalculator::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 worldPos = glm::vec2(
        p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return worldPos;
}