#include "PathfindingManager.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>
#include <algorithm>

PathfindingManager::PathfindingManager(LabyrinthManager& labyrinthManager)
    : m_labyrinthManager(labyrinthManager) {}

bool PathfindingManager::IsWalkable(int x, int y) const {
    int tileID = m_labyrinthManager.GetTile(x, y);

    // Check if the tile is valid and walkable
    switch (tileID) {
        case Tile::FloorSmallSquares:
        case Tile::FloorSmallSquaresGold:
        case Tile::FloorSpiral:
        case Tile::FloorSpiralGold:
        case Tile::FloorSquare:
        case Tile::FloorSquareGold:
        case Tile::Grass:
            return true; // Walkable tiles
        default:
            return false; // Non-walkable tiles (e.g., walls, empty)
    }
}

int PathfindingManager::CalculateHeuristic(const glm::ivec2& start, const glm::ivec2& end) const {
    // Manhattan distance for a grid-based pathfinding
    return abs(start.x - end.x) + abs(start.y - end.y);
}

std::vector<glm::ivec2> PathfindingManager::GetNeighbors(const glm::ivec2& position) const {
    std::vector<glm::ivec2> neighbors;
    static const glm::ivec2 directions[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    for (const auto& dir : directions) {
        glm::ivec2 neighbor = position + dir;
        if (IsWalkable(neighbor.x, neighbor.y)) {
            neighbors.push_back(neighbor);
        }
    }
    return neighbors;
}

std::vector<glm::ivec2> PathfindingManager::FindPath(const glm::ivec2& start, const glm::ivec2& end) {
    using NodePtr = Node*;

    // Priority queue for the open set, sorted by fCost
    std::priority_queue<NodePtr, std::vector<NodePtr>, std::function<bool(NodePtr, NodePtr)>> openList(
        [](NodePtr a, NodePtr b) { return a->fCost > b->fCost; });

    // Data structures for A* algorithm
    std::unordered_map<glm::ivec2, Node> allNodes;
    std::unordered_set<glm::ivec2> closedList;

    // Initialize the start node
    Node& startNode = allNodes[start] = { start, 0, CalculateHeuristic(start, end), 0, nullptr };
    startNode.fCost = startNode.gCost + startNode.hCost;
    openList.push(&startNode);

    while (!openList.empty()) {
        Node* current = openList.top();
        openList.pop();

        // Goal reached
        if (current->position == end) {
            std::vector<glm::ivec2> path;
            for (Node* node = current; node != nullptr; node = node->parent) {
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
                neighborNode.position = neighborPos;
                neighborNode.gCost = tentativeGCost;
                neighborNode.hCost = CalculateHeuristic(neighborPos, end);
                neighborNode.fCost = neighborNode.gCost + neighborNode.hCost;
                neighborNode.parent = current;

                // Add to open list if not already added
                if (!neighborNode.parent || tentativeGCost < neighborNode.gCost) {
                    openList.push(&neighborNode);
                }
            }
        }
    }

    return {}; // Return an empty path if no path is found
}
