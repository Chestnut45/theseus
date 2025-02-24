//-----------------------------------------------------------------------------
// File:            PathfindingManager.h
// Original Author: Youssef Ashraf
// ver 1.3.
//-----------------------------------------------------------------------------

#ifndef PATHFINDING_MANAGER_H
#define PATHFINDING_MANAGER_H

#include "LabyrinthManager.h"
#include "W_GameObject.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

class PathfindingManager
{
public:
    explicit PathfindingManager(LabyrinthManager* labyrinthManager);

    // Registers an entity for pathfinding
    void RegisterEntity(wolf::GameObject* entity);

    // Updates all registered entities' movement based on pathfinding
    void UpdateEntities(float delta);

    // Finds the optimal path from start to goal
    std::vector<glm::ivec2> FindPath(const glm::ivec2& start, const glm::ivec2& goal);

    // Checks if a tile is walkable
    bool IsTileWalkable(int x, int y) const;

    LabyrinthManager* GetLabyrinthManager() const { return m_labyrinthManager; }

    // Returns the neighbors of a given node
    std::vector<glm::ivec2> GetNeighbors(const glm::ivec2& node) const;
    struct EntityPathData
    {
        wolf::GameObject* entity = nullptr;
        glm::ivec2 currentTile;
        glm::ivec2 targetTile;
        std::vector<glm::ivec2> path;
    };
    // Returns the path data for a registered entity
    PathfindingManager::EntityPathData& GetPathData(wolf::GameObject* entity);
    private:


    // Helper function to reconstruct a path from the "cameFrom" map
    std::vector<glm::ivec2> ReconstructPath(
        const std::unordered_map<glm::ivec2, glm::ivec2>& cameFrom,
        const glm::ivec2& current) const;



    LabyrinthManager* m_labyrinthManager;
    std::unordered_map<wolf::GameObject*, EntityPathData> m_registeredEntities;
    std::unordered_set<glm::ivec2> m_reservedTiles;
};

#endif // PATHFINDING_MANAGER_H
