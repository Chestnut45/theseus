#pragma once

#include "glm_ivec2_hash.h"
#include <vector>
#include "LabyrinthManager.h"
#include <unordered_map>


// A* Pathfinding Manager for navigating a grid-based labyrinth
class PathfindingManager {
public:
    explicit PathfindingManager(LabyrinthManager& labyrinthManager);

    // Finds a path from start to end positions (in tile coordinates)
    std::vector<glm::ivec2> FindPath(const glm::ivec2& start, const glm::ivec2& end);

private:
    struct Node {
        glm::ivec2 position;
        int gCost, hCost, fCost;
        Node* parent;

        bool operator>(const Node& other) const { return fCost > other.fCost; }
    };

    // Helper functions
    bool IsWalkable(int x, int y) const;
    int CalculateHeuristic(const glm::ivec2& start, const glm::ivec2& end) const;
    std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& position) const;

    LabyrinthManager& m_labyrinthManager;
};
