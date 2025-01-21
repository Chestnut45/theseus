#pragma once

#include "LabyrinthManager.h"
#include <glm/glm.hpp>
#include <queue>
#include <unordered_map>
#include <vector>
#include <algorithm>

class PathfindingManager
{
public:
    // Constructor
    PathfindingManager(const LabyrinthManager& labyrinthManager);
    ~PathfindingManager() = default;

    // Disable copying and moving
    PathfindingManager(const PathfindingManager&) = delete;
    PathfindingManager& operator=(const PathfindingManager&) = delete;

    // Finds the shortest path using the A* algorithm
    std::vector<glm::ivec2> FindPath(const glm::ivec2& start, const glm::ivec2& goal);



private:
    const LabyrinthManager& m_labyrinthManager;

    // Helper methods
    bool IsTileWalkable(int x, int y) const;
    std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& node) const;
    float Heuristic(const glm::ivec2& a, const glm::ivec2& b) const;
    float Distance(const glm::ivec2& a, const glm::ivec2& b) const;
    std::vector<glm::ivec2> ReconstructPath(
        const std::unordered_map<glm::ivec2, glm::ivec2>& cameFrom,
        const glm::ivec2& current) const;
};
