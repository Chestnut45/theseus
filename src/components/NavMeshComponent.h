#pragma once

#include "PathfindingManager.h"
#include "LabyrinthManager.h"
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Forward declare std::hash specialization for std::pair<int, int> before any usage
namespace std
{
    template<>
    struct hash<std::pair<int, int>>
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
        std::vector<glm::vec2> vertices;     // Polygon vertices in world space
        std::vector<int> neighbors;          // Indices of adjacent polygons
        glm::vec2 center;                    // Center point of polygon
        int id;                              // Unique identifier
        bool walkable = true;                // Is this area traversable?
        bool visible = true;                 // Should this polygon be rendered?
    };

    struct NavEdge
    {
        glm::vec2 start;                     // Edge start point
        glm::vec2 end;                       // Edge end point
        int poly1;                           // First polygon
        int poly2;                           // Second polygon (-1 if boundary)
    };

    NavMeshComponent();
    ~NavMeshComponent();

    // Initialize with PathfindingManager
    void Init(PathfindingManager* pathfindingManager);

    // Generate NavMesh from the labyrinth grid
    void GenerateFromLabyrinth(LabyrinthManager* labyrinthManager);

    // Generate NavMesh from custom polygons
    void GenerateFromPolygons(const std::vector<NavPolygon>& polygons);

    // Find a path through the NavMesh
    std::vector<glm::vec2> FindPath(const glm::vec2& start, const glm::vec2& goal);

    // Find the polygon containing a point
    int FindPolygon(const glm::vec2& point) const;

    // Get nearest point on NavMesh
    glm::vec2 GetNearestPointOnNavMesh(const glm::vec2& point) const;

    // Debug visualization
    void DebugDraw() const;

    // Update function
    void Update(float delta);

    bool IsDebugDrawEnabled() const { return m_debugDrawEnabled; }
    
    // Update dynamic obstacles
    void UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles);

private:
    // Constants
    static constexpr float SPATIAL_CELL_SIZE = 128.0f;  // Size of spatial hash cells

    // Data
    PathfindingManager* m_pPathfindingManager = nullptr;
    std::vector<NavPolygon> m_polygons;
    std::vector<NavEdge> m_edges;
    std::vector<wolf::GameObject*> m_obstacles;
    
    // Spatial hash for fast polygon lookups
    // Key: Cell coordinates (x, y), Value: List of polygon indices in this cell
    using SpatialKey = std::pair<int, int>;
    std::unordered_map<SpatialKey, std::vector<int>> m_spatialHash;
    
    // Track polygons affected by obstacles for efficient updates
    std::vector<int> m_obstacleAffectedPolygons;
    
    // For handling occupied tiles
    std::unordered_map<int, bool> m_originalVisibility;

    // Helper methods
    bool IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const;
    std::vector<int> FindPolygonPath(int startPoly, int endPoly) const;
    std::vector<glm::vec2> SmoothPath(const std::vector<glm::vec2>& path) const;
    float DistancePointToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    glm::vec2 ProjectPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    bool LineOfSight(const glm::vec2& start, const glm::vec2& end) const;
    
    // Improved funnel algorithm for path finding
    std::vector<glm::vec2> ImprovedFunnelAlgorithm(
        const std::vector<int>& corridorPolygons,
        const glm::vec2& start,
        const glm::vec2& end) const;

    bool m_debugDrawEnabled = false;

    inline void ProcessAffectedPolygon(int polyId, bool isPlayer, std::unordered_set<int>& affectedPolygons);

};