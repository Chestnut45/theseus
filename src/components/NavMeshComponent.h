#ifndef NAVMESH_COMPONENT_H
#define NAVMESH_COMPONENT_H

#include "PathfindingManager.h"
#include "W_BaseComponent.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

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

    //update function tehee
    void Update(float delta);

    bool IsDebugDrawEnabled() const { return m_debugDrawEnabled; }
    void UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles);


private:
    PathfindingManager* m_pPathfindingManager = nullptr;
    std::vector<NavPolygon> m_polygons;
    std::vector<NavEdge> m_edges;
    std::vector<wolf::GameObject*> m_obstacles;

    // Helper methods
    bool IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const;
    std::vector<int> FindPolygonPath(int startPoly, int endPoly) const;
    std::vector<glm::vec2> SmoothPath(const std::vector<glm::vec2>& path) const;
    float DistancePointToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    glm::vec2 ProjectPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const;
    bool LineOfSight(const glm::vec2& start, const glm::vec2& end) const;
    
    // Funnel algorithm for path finding
    std::vector<glm::vec2> FunnelAlgorithm(
        const std::vector<int>& corridorPolygons,
        const glm::vec2& start,
        const glm::vec2& end) const;

    bool m_debugDrawEnabled = false;



};

#endif