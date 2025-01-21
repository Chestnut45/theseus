#include "PathfindingManager.h"
#include <cmath> // for abs and sqrt

PathfindingManager::PathfindingManager(const LabyrinthManager& labyrinthManager)
    : m_labyrinthManager(labyrinthManager) {}

bool PathfindingManager::IsTileWalkable(int x, int y) const
{
    int tileID = m_labyrinthManager.GetTile(x, y);
    if (tileID < 0) // Out of bounds or empty
        return false;

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
    // Include both cardinal and diagonal directions
    std::vector<glm::ivec2> directions = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},   // Cardinal directions
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1} // Diagonal directions
    };

    for (const auto& dir : directions)
    {
        glm::ivec2 neighbor = node + dir;
        if (IsTileWalkable(neighbor.x, neighbor.y))
        {
            neighbors.push_back(neighbor);
        }
    }

    return neighbors;
}

float PathfindingManager::Heuristic(const glm::ivec2& a, const glm::ivec2& b) const
{
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx + dy) + (std::sqrt(2.0f) - 2.0f) * std::min(dx, dy);
}

float PathfindingManager::Distance(const glm::ivec2& a, const glm::ivec2& b) const
{
    if (a.x != b.x && a.y != b.y) // Diagonal movement
    {
        return 1.414f; // Approximation of √2
    }
    return 1.0f; // Cardinal movement
}

std::vector<glm::ivec2> PathfindingManager::ReconstructPath(
    const std::unordered_map<glm::ivec2, glm::ivec2>& cameFrom,
    const glm::ivec2& current) const
{
    std::vector<glm::ivec2> path;
    glm::ivec2 currentNode = current;

    while (cameFrom.find(currentNode) != cameFrom.end())
    {
        path.push_back(currentNode);
        currentNode = cameFrom.at(currentNode);
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<glm::ivec2> PathfindingManager::FindPath(
    const glm::ivec2& start, const glm::ivec2& goal)
{
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
            return ReconstructPath(cameFrom, current);
        }

        for (const auto& neighbor : GetNeighbors(current))
        {
            float tentativeG = gScore[current] + Distance(current, neighbor);

            if (tentativeG < gScore[neighbor] || gScore.find(neighbor) == gScore.end())
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeG;
                fScore[neighbor] = tentativeG + Heuristic(neighbor, goal);

                openSet.emplace(neighbor, fScore[neighbor]);
            }
        }
    }

    return {}; // Return an empty path if no path is found
}
