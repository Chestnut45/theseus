#include "PathfindingManager.h"
#include <cmath> // for abs and sqrt

PathfindingManager::PathfindingManager(const LabyrinthManager& labyrinthManager)
    : m_labyrinthManager(labyrinthManager) {}

bool PathfindingManager::IsTileWalkable(int x, int y) const
{
    int tileID = m_labyrinthManager.GetTile(x, y);

    if (tileID < 0) // Out of bounds
    {
        printf("Out of bounds or invalid tile: (%d, %d)\n", x, y);
        return false;
    }

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
        printf("Unwalkable tile ID: %d at (%d, %d)\n", tileID, x, y);
        return false;
    }
}

std::vector<glm::ivec2> PathfindingManager::GetNeighbors(const glm::ivec2& node) const
{
    std::vector<glm::ivec2> neighbors;
    std::vector<glm::ivec2> directions = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0}, // Cardinal directions
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // Diagonal directions
    };

    for (const auto& dir : directions)
    {
        glm::ivec2 neighbor = node + dir;

        // Check for walkability
        if (IsTileWalkable(neighbor.x, neighbor.y))
        {
            // Disallow diagonal movement if adjacent walls block the path
            if ((dir.x != 0 && dir.y != 0) &&  // Diagonal direction
                (!IsTileWalkable(node.x + dir.x, node.y) || !IsTileWalkable(node.x, node.y + dir.y)))
            {
                continue; // Skip this diagonal neighbor
            }

            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}


float PathfindingManager::Heuristic(const glm::ivec2& a, const glm::ivec2& b) const
{
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);

    // Octile distance
    float diagonalCost = std::sqrt(2.0f);
    return diagonalCost * std::min(dx, dy) + std::abs(dx - dy);
}

float PathfindingManager::Distance(const glm::ivec2& a, const glm::ivec2& b) const
{
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);

    // Diagonal movement cost is sqrt(2), cardinal movement cost is 1
    if (dx > 0 && dy > 0)
        return std::sqrt(2.0f); // Diagonal movement
    else
        return 1.0f; // Cardinal movement
}

std::vector<glm::ivec2> PathfindingManager::ReconstructPath(
    const std::unordered_map<glm::ivec2, glm::ivec2>& cameFrom,
    const glm::ivec2& current) const
{
    std::vector<glm::ivec2> path;
    glm::ivec2 currentNode = current;
    int maxIterations = 1000; // Prevent infinite loops

    while (cameFrom.find(currentNode) != cameFrom.end())
    {
        if (path.size() >= maxIterations)
        {
            // printf("Error: Path reconstruction exceeded maximum iterations.\n");
            return {};
        }

        path.push_back(currentNode);
        currentNode = cameFrom.at(currentNode);

        // printf("Reconstructing path: (%d, %d)\n", currentNode.x, currentNode.y);
    }

    if (path.empty() || cameFrom.find(path.front()) == cameFrom.end())
    {
        // printf("Error: Path reconstruction failed. Invalid 'cameFrom' map.\n");
        return {};
    }

    std::reverse(path.begin(), path.end());
    return path;
}


std::vector<glm::ivec2> PathfindingManager::FindPath(
    const glm::ivec2& start, const glm::ivec2& goal)
{
    // printf("Starting pathfinding from (%d, %d) to (%d, %d)\n", start.x, start.y, goal.x, goal.y);

    if (!IsTileWalkable(start.x, start.y))
    {
        // printf("Error: Start tile (%d, %d) is not walkable.\n", start.x, start.y);
        return {};
    }

    if (!IsTileWalkable(goal.x, goal.y))
    {
        // printf("Error: Goal tile (%d, %d) is not walkable.\n", goal.x, goal.y);
        return {};
    }

    if (start == goal)
    {
        // printf("Start and goal are the same tile (%d, %d). Returning empty path.\n", start.x, start.y);
        return {start};
    }

    using Node = std::pair<glm::ivec2, float>;

    auto compare = [](const Node& a, const Node& b) {
        return a.second > b.second;
    };

    std::priority_queue<Node, std::vector<Node>, decltype(compare)> openSet(compare);
    std::unordered_map<glm::ivec2, glm::ivec2> cameFrom;
    std::unordered_map<glm::ivec2, float> gScore, fScore;

    gScore[start] = 0.0f;
    fScore[start] = Heuristic(start, goal);
    openSet.emplace(start, fScore[start]);

    while (!openSet.empty())
    {
        glm::ivec2 current = openSet.top().first;
        openSet.pop();

        if (current == goal)
        {
            // printf("Path found!\n");
            return ReconstructPath(cameFrom, current);
        }

        for (const auto& neighbor : GetNeighbors(current))
        {
            if (gScore.find(neighbor) == gScore.end())
            {
                gScore[neighbor] = std::numeric_limits<float>::max();
            }

            float tentativeG = gScore[current] + Distance(current, neighbor);

            if (tentativeG < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeG;
                fScore[neighbor] = tentativeG + Heuristic(neighbor, goal);

                openSet.emplace(neighbor, fScore[neighbor]);
            }
        }
    }

    // printf("No path found from (%d, %d) to (%d, %d)\n", start.x, start.y, goal.x, goal.y);
    return {}; // Return an empty path if no path is found
}