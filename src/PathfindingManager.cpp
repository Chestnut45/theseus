#include "PathfindingManager.h"
#include "MinitaurController.h"
#include <cmath> // for abs and sqrt
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp> // For squared length comparisons
#include <random> // For slight push adjustments

PathfindingManager::PathfindingManager(LabyrinthManager* labyrinthManager)
{
    if (!labyrinthManager)
    {
        printf("Error: PathfindingManager received a nullptr for LabyrinthManager!\n");
    }
    m_labyrinthManager = labyrinthManager;
}

bool PathfindingManager::IsTileWalkable(int x, int y) const
{
    int tileID = m_labyrinthManager->GetTile(x, y);
    if (tileID < 0) return false; // Out of bounds

    switch (tileID)
    {
    case Tile::BorderedGrass:
    case Tile::Bricks:
    case Tile::FloorSmallSquares:
    case Tile::FloorSmallSquaresGold:
    case Tile::FloorSpiralGold:
    case Tile::FloorSpiral:
    case Tile::FloorSquareGold:
    case Tile::FloorSquare:
    case Tile::Grass:
        return true;
    default:
        return false;
    }
}

std::vector<glm::ivec2> PathfindingManager::GetNeighbors(const glm::ivec2& node) const
{
    std::vector<glm::ivec2> neighbors;
    std::vector<glm::ivec2> directions = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0}, 
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    for (const auto& dir : directions)
    {
        glm::ivec2 neighbor = node + dir;
        if (IsTileWalkable(neighbor.x, neighbor.y))
        {
            if ((dir.x != 0 && dir.y != 0) && 
                (!IsTileWalkable(node.x + dir.x, node.y) || !IsTileWalkable(node.x, node.y + dir.y)))
            {
                continue; // Skip diagonal if adjacent walls exist
            }
            neighbors.push_back(neighbor);
        }
    }
    return neighbors;
}

std::vector<glm::ivec2> PathfindingManager::FindPath(const glm::ivec2& start, const glm::ivec2& goal)
{
    if (!IsTileWalkable(start.x, start.y) || !IsTileWalkable(goal.x, goal.y))
        return {};

    if (start == goal)
        return {start};

    using Node = std::pair<glm::ivec2, float>; // (tile, cost)
    auto compare = [](const Node& a, const Node& b) { return a.second > b.second; };
    std::priority_queue<Node, std::vector<Node>, decltype(compare)> openSet(compare);
    std::unordered_map<glm::ivec2, glm::ivec2> cameFrom;
    std::unordered_map<glm::ivec2, float> gScore, fScore;

    gScore[start] = 0.0f;
    fScore[start] = glm::distance(glm::vec2(start), glm::vec2(goal));
    openSet.emplace(start, fScore[start]);

    while (!openSet.empty())
    {
        glm::ivec2 current = openSet.top().first;
        openSet.pop();

        if (current == goal)
            return ReconstructPath(cameFrom, current);

        for (const auto& neighbor : GetNeighbors(current))
        {
            float tentativeG = gScore[current] + glm::distance(glm::vec2(current), glm::vec2(neighbor));

            if (gScore.find(neighbor) == gScore.end() || tentativeG < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeG;
                fScore[neighbor] = tentativeG + glm::distance(glm::vec2(neighbor), glm::vec2(goal));
                openSet.emplace(neighbor, fScore[neighbor]);
            }
        }
    }
    return {}; // No path found
}

void PathfindingManager::UpdateEntities(float delta)
{
    constexpr float TILE_CENTER_OFFSET = 48.0f;
    constexpr float TOLERANCE = 2.0f;
    constexpr float SEPARATION_FORCE = 30.0f;
    constexpr float REPATH_INTERVAL = 0.5f;
    constexpr float STUCK_TIME_THRESHOLD = 0.6f;
    constexpr float REPATH_DELAY = 0.1f; // Delay before checking for new paths

    std::unordered_map<glm::ivec2, std::vector<wolf::GameObject*>> tileOccupationMap;
    std::unordered_map<glm::ivec2, wolf::GameObject*> tileOwners;

    static std::unordered_map<wolf::GameObject*, float> lastRepathTime;
    static std::unordered_map<wolf::GameObject*, float> stuckTimeTracker;
    static float globalTime = 0.0f;
    globalTime += delta;

    for (auto& [entity, data] : m_registeredEntities)
    {
        auto* minitaurController = entity->GetComponent<MinitaurController>();
        if (!minitaurController || minitaurController->GetState() != MinitaurController::EnemyState::CHASING)
            continue;

        glm::ivec2 currentTile = glm::ivec2(entity->GetComponent<wolf::Transform2D>()->GetGlobalPosition()) /
                                 (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        glm::ivec2 targetTile = glm::ivec2(minitaurController->GetTarget()->GetComponent<wolf::Transform2D>()->GetGlobalPosition()) /
                                (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        // **Track how long the entity has been stuck**
        if (data.currentTile == currentTile)
        {
            stuckTimeTracker[entity] += delta;
        }
        else
        {
            stuckTimeTracker[entity] = 0.0f;
        }

        // **Check if we need to re-path**
        if ((data.path.empty() || targetTile != data.targetTile) &&
            (globalTime - lastRepathTime[entity] > REPATH_INTERVAL))
        {
            std::vector<glm::ivec2> newPath = FindPath(currentTile, targetTile);

            // **Check if the new path is identical to the previous one**
            if (newPath == data.path && stuckTimeTracker[entity] > REPATH_DELAY)
            {
                // Force slight variation by picking an adjacent starting tile
                std::vector<glm::ivec2> neighbors = GetNeighbors(currentTile);
                for (const auto& neighbor : neighbors)
                {
                    if (m_reservedTiles.find(neighbor) == m_reservedTiles.end())
                    {
                        newPath = FindPath(neighbor, targetTile);
                        if (!newPath.empty()) break;
                    }
                }
            }

            data.path = newPath;
            data.targetTile = targetTile;
            lastRepathTime[entity] = globalTime;
        }

        // **Track occupied tiles & prevent multiple entities from moving to the same tile**
        if (!data.path.empty())
        {
            glm::ivec2 nextTile = data.path.front();

            if (tileOwners.find(nextTile) != tileOwners.end() && tileOwners[nextTile] != entity)
            {
                // Conflict detected: force alternative pathing
                data.path = FindPath(currentTile, targetTile);
                if (!data.path.empty())
                {
                    nextTile = data.path.front();
                }
            }

            tileOwners[nextTile] = entity;
            tileOccupationMap[nextTile].push_back(entity);
        }
    }

    // **Handle entity separation using glm::mix**
    for (auto& [tile, entities] : tileOccupationMap)
    {
        if (entities.size() >= 2)
        {
            glm::vec2 avgPosition = glm::vec2(0.0f);
            for (auto* entity : entities)
            {
                avgPosition += entity->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            }
            avgPosition /= entities.size();

            for (auto* entity : entities)
            {
                glm::vec2 entityPosition = entity->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::vec2 separationDirection = glm::normalize(entityPosition - avgPosition);
                glm::vec2 softPush = separationDirection * SEPARATION_FORCE * delta;

                glm::vec2 newPos = glm::mix(entityPosition, entityPosition + softPush, 0.5f);
                entity->GetComponent<wolf::Transform2D>()->SetPosition(newPos);
            }
        }
    }

    // **Move Entities Smoothly**
    for (auto& [entity, data] : m_registeredEntities)
    {
        if (!data.path.empty())
        {
            glm::ivec2 nextTile = data.path.front();
            glm::vec2 nextTileWorldPos = m_labyrinthManager->GetWorldPosition(nextTile) + glm::vec2(TILE_CENTER_OFFSET, TILE_CENTER_OFFSET);
            auto* transform = entity->GetComponent<wolf::Transform2D>();
            if (!transform) 
            {
                continue;
            }
            
            glm::vec2 currentPosition = transform->GetGlobalPosition();
             glm::vec2 direction = nextTileWorldPos - currentPosition;

            if (glm::length(direction) > TOLERANCE)
            {
                direction = glm::normalize(direction);
                entity->GetComponent<VelocityComponent>()->SetVelocity(direction * 125.0f);
            }
            else
            {
                data.path.erase(data.path.begin());
                data.currentTile = nextTile;
            }
        }
    }
}



void PathfindingManager::RegisterEntity(wolf::GameObject* entity)
{
    if (!entity) return;
    
    if (m_registeredEntities.find(entity) == m_registeredEntities.end())
    {
        glm::ivec2 startTile = glm::ivec2(entity->GetComponent<wolf::Transform2D>()->GetGlobalPosition()) /
                               (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
                               
        glm::ivec2 targetTile = glm::ivec2(entity->GetComponent<MinitaurController>()->GetTarget()->GetComponent<wolf::Transform2D>()->GetGlobalPosition()) /
                                (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
                                
        m_registeredEntities[entity] = {entity, startTile, targetTile, {}}; // No path yet
    }
}

std::vector<glm::ivec2> PathfindingManager::ReconstructPath(
    const std::unordered_map<glm::ivec2, glm::ivec2>& cameFrom,
    const glm::ivec2& current) const
{
    std::vector<glm::ivec2> path;
    glm::ivec2 currentNode = current;

    // Ensure that the path exists in the map
    if (cameFrom.find(currentNode) == cameFrom.end())
    {
        printf("Warning: Path reconstruction failed. No previous node found.\n");
        return {};
    }

    while (cameFrom.find(currentNode) != cameFrom.end())
    {
        path.push_back(currentNode);
        currentNode = cameFrom.at(currentNode);

        // Safety check to prevent infinite loops
        if (path.size() > 500) // Arbitrary large limit to detect loops
        {
            printf("Error: Path reconstruction exceeded safe iteration count.\n");
            return {};
        }
    }

    std::reverse(path.begin(), path.end());
    return path;
}

const PathfindingManager::EntityPathData& PathfindingManager::GetPathData(wolf::GameObject* entity) const
{
    static const EntityPathData emptyData{}; // Ensures we don't return a reference to a temporary object

    auto it = m_registeredEntities.find(entity);
    return (it != m_registeredEntities.end()) ? it->second : emptyData;
}


