#pragma once

#include "PathfindingManager.h"
#include "LabyrinthManager.h"
#include "W_BaseComponent.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Hash for std::pair<int, int>
namespace std
{
    template<> struct hash<std::pair<int, int>>
    {
        std::size_t operator()(const std::pair<int, int>& p) const
        {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };
}

class NavMeshComponent : public wolf::BaseComponent
{
public:
    struct NavPolygon
    {
        std::vector<glm::vec2> vertices;  // Polygon vertices in world space
        std::vector<int> neighbors;       // Indices of adjacent polygons
        glm::vec2 center;                 // Center point of polygon
        int id;                           // Unique identifier
        bool walkable = true;             // Is this area traversable?
        bool visible = true;              // Should this polygon be rendered?
    };

    struct NavEdge
    {
        glm::vec2 start, end;             // Edge endpoints
        int poly1, poly2;                 // Connected polygons (-1 for boundary)
    };

    NavMeshComponent();
    ~NavMeshComponent();

    void Init(PathfindingManager* pathfindingManager);
    void GenerateFromLabyrinth(LabyrinthManager* labyrinthManager);
    void Update(float delta);
    void UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles);
    void Clear();

    // Pathfinding functions
    std::vector<glm::vec2> FindPath(const glm::vec2& start, const glm::vec2& goal);
    bool IsPathValid(const std::vector<glm::vec2>& path) const;
    std::vector<glm::vec2> CreateGridBasedPath(const glm::vec2& start, const glm::vec2& goal);

    // Helper functions
    int FindPolygon(const glm::vec2& point) const;
    glm::vec2 GetNearestPointOnNavMesh(const glm::vec2& point) const;

    // Debug
    void DebugDraw() const;
    bool IsDebugDrawEnabled() const { return m_debugDrawEnabled; }

private:
    static constexpr float SPATIAL_CELL_SIZE = 128.0f;

    // Data members
    PathfindingManager* m_pPathfindingManager = nullptr;
    std::vector<NavPolygon> m_polygons;
    std::vector<NavEdge> m_edges;
    std::vector<wolf::GameObject*> m_obstacles;
    std::vector<int> m_obstacleAffectedPolygons;
    std::unordered_map<int, bool> m_originalVisibility;
    bool m_debugDrawEnabled = false;
    
    // Spatial partitioning
    using SpatialKey = std::pair<int, int>;
    std::unordered_map<SpatialKey, std::vector<int>> m_spatialHash;

    // Helper methods
    bool IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const;
    std::vector<int> FindPolygonPath(int startPoly, int endPoly) const;
    std::vector<glm::vec2> SmoothPath(const std::vector<glm::vec2>& path) const;
    float DistancePointToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    glm::vec2 ProjectPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    bool LineOfSight(const glm::vec2& start, const glm::vec2& end) const;
    std::vector<glm::vec2> ImprovedFunnelAlgorithm(
        const std::vector<int>& polygonPath,
        const glm::vec2& start,
        const glm::vec2& end) const;
    void ProcessAffectedPolygon(int polyId, bool isPlayer, std::unordered_set<int>& affectedPolygons);
};