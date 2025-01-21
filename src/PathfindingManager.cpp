#include "PathfindingManager.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iostream> // For debugging logs

PathfindingManager::PathfindingManager(LabyrinthManager& labyrinthManager)
    : m_labyrinthManager(labyrinthManager),
      m_walkableTiles({
          Tile::BorderedGrass, Tile::Bricks, Tile::FloorSmallSquares,
          Tile::FloorSmallSquaresGold, Tile::FloorSpiralGold,
          Tile::FloorSpiral, Tile::FloorSquareGold,
          Tile::FloorSquare, Tile::Grass}) {}

glm::ivec2 PathfindingManager::GetTilePosition(const glm::vec2& worldPosition) const {
    return m_labyrinthManager.GetTilePosition(worldPosition);
}

glm::vec2 PathfindingManager::GetTileWorldPosition(const glm::ivec2& tilePosition) const {
    // Use LabyrinthManager constants to calculate the world position
    const int tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    return glm::vec2(tilePosition) * glm::vec2(tileSize);
}

void PathfindingManager::UpdateWalkableTiles(const std::unordered_set<Tile::type>& newWalkableTiles) {
    m_walkableTiles = newWalkableTiles;
}

void PathfindingManager::SetDiagonalMovement(bool enable) {
    m_allowDiagonalMovement = enable;
}

bool PathfindingManager::IsWalkable(const glm::ivec2& position) const {
    int tileID = m_labyrinthManager.GetTile(position.x, position.y);
    return m_walkableTiles.count(tileID) > 0;
}

int PathfindingManager::CalculateHeuristic(const glm::ivec2& start, const glm::ivec2& end) const {
    if (m_allowDiagonalMovement) {
        // Euclidean distance for diagonal movement
        return static_cast<int>(glm::distance(glm::vec2(start), glm::vec2(end)));
    }
    // Manhattan distance for orthogonal movement
    return abs(start.x - end.x) + abs(start.y - end.y);
}

std::vector<glm::ivec2> PathfindingManager::GetNeighbors(const glm::ivec2& position) const {
    static const glm::ivec2 orthogonalDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    static const glm::ivec2 diagonalDirections[] = {{1, 1}, {-1, -1}, {1, -1}, {-1, 1}};

    std::vector<glm::ivec2> neighbors;

    // Add orthogonal neighbors
    for (const auto& dir : orthogonalDirections) {
        glm::ivec2 neighbor = position + dir;
        if (IsWalkable(neighbor)) {
            neighbors.push_back(neighbor);
        }
    }

    // Add diagonal neighbors if enabled
    if (m_allowDiagonalMovement) {
        for (const auto& dir : diagonalDirections) {
            glm::ivec2 neighbor = position + dir;
            if (IsWalkable(neighbor)) {
                neighbors.push_back(neighbor);
            }
        }
    }

    return neighbors;
}

std::vector<glm::ivec2> PathfindingManager::FindPath(const glm::ivec2& start, const glm::ivec2& end) {
    using NodePtr = Node*;

    // Validate start and end positions
    if (!IsWalkable(start) || !IsWalkable(end)) {
        std::cerr << "Invalid start or end position for pathfinding.\n";
        return {};
    }

    // Priority queue for the open set
    std::priority_queue<NodePtr, std::vector<NodePtr>, std::greater<>> openList;
    std::unordered_map<glm::ivec2, Node> allNodes;
    std::unordered_set<glm::ivec2> closedList;

    // Initialize the start node
    Node& startNode = allNodes[start] = {start, 0, CalculateHeuristic(start, end), 0, nullptr};
    startNode.fCost = startNode.gCost + startNode.hCost;
    openList.push(&startNode);

    while (!openList.empty()) {
        Node* current = openList.top();
        openList.pop();

        if (current->position == end) {
            std::vector<glm::ivec2> path;
            for (Node* node = current; node; node = node->parent) {
                path.push_back(node->position);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        closedList.insert(current->position);

        for (const auto& neighborPos : GetNeighbors(current->position)) {
            if (closedList.count(neighborPos)) continue;

            int tentativeGCost = current->gCost + 1;
            Node& neighborNode = allNodes[neighborPos];

            if (!neighborNode.parent || tentativeGCost < neighborNode.gCost) {
                neighborNode = {neighborPos, tentativeGCost, CalculateHeuristic(neighborPos, end), 0, current};
                neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
                openList.push(&neighborNode);
            }
        }
    }

    std::cerr << "Pathfinding failed: No path found.\n";
    return {};
}
