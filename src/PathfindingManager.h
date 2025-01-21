#pragma once

#include <vector>
#include <unordered_set>
#include <glm/glm.hpp>
#include "LabyrinthManager.h"
#include "LabyrinthTiles.h"

class PathfindingManager {
public:
    explicit PathfindingManager(LabyrinthManager& labyrinthManager);

    // Finds a path from start to end positions (in tile coordinates)
    std::vector<glm::ivec2> FindPath(const glm::ivec2& start, const glm::ivec2& end);

    // Updates walkable tiles dynamically
    void UpdateWalkableTiles(const std::unordered_set<Tile::type>& newWalkableTiles);

    // Enables or disables diagonal movement
    void SetDiagonalMovement(bool enable);

    // Converts tile coordinates to world position
    glm::vec2 GetTileWorldPosition(const glm::ivec2& tilePosition) const;

    // Converts world position to tile coordinates
    glm::ivec2 GetTilePosition(const glm::vec2& worldPosition) const;

private:
    struct Node {
        glm::ivec2 position;
        int gCost, hCost, fCost;
        Node* parent = nullptr;

        bool operator>(const Node& other) const { return fCost > other.fCost; }
    };

    // Helper functions
    bool IsWalkable(const glm::ivec2& position) const;
    int CalculateHeuristic(const glm::ivec2& start, const glm::ivec2& end) const;
    std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& position) const;

    LabyrinthManager& m_labyrinthManager;
    std::unordered_set<Tile::type> m_walkableTiles;
    bool m_allowDiagonalMovement = false;
};
