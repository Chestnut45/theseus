// NavMeshComponent.cpp
#include "NavMeshComponent.h"
#include <algorithm>
#include <queue>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <GLShapesRenderer.h>
#include <W_Transform2D.h>

NavMeshComponent::NavMeshComponent()
{
}

NavMeshComponent::~NavMeshComponent()
{
}

void NavMeshComponent::Update(float delta)
{
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_7))
    {
        m_debugDrawEnabled = !m_debugDrawEnabled;
        printf("NavMesh Debug Drawing: %s\n", m_debugDrawEnabled ? "Enabled" : "Disabled");
    }
    
}

void NavMeshComponent::Init(PathfindingManager* pathfindingManager)
{
    m_pPathfindingManager = pathfindingManager;
}

void NavMeshComponent::GenerateFromLabyrinth(LabyrinthManager* labyrinthManager)
{
    // Clear existing data
    m_polygons.clear();
    m_edges.clear();
    
    const int width = labyrinthManager->GetWidth();
    const int height = labyrinthManager->GetHeight();
    
    // This is a very basic implementation that creates square polygons for each walkable tile
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (m_pPathfindingManager->IsTileWalkable(x, y))
            {
                NavPolygon poly;
                poly.id = m_polygons.size();
                
                // Calculate world coordinates for the polygon vertices
                glm::vec2 topLeft = labyrinthManager->GetWorldPosition(glm::ivec2(x, y));
                float tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
                
                // Define polygon vertices (clockwise)
                poly.vertices = {
                    topLeft,
                    topLeft + glm::vec2(tileSize, 0),
                    topLeft + glm::vec2(tileSize, tileSize),
                    topLeft + glm::vec2(0, tileSize)
                };
                
                // Calculate center
                poly.center = topLeft + glm::vec2(tileSize / 2.0f, tileSize / 2.0f);
                
                // Find neighbors
                if (x > 0 && m_pPathfindingManager->IsTileWalkable(x - 1, y))
                {
                    // Find polygon index for the neighbor to the left
                    int neighborId = y * width + (x - 1);
                    if (neighborId < static_cast<int>(m_polygons.size()))
                    {
                        poly.neighbors.push_back(neighborId);
                        m_polygons[neighborId].neighbors.push_back(poly.id);
                        
                        // Create an edge
                        NavEdge edge;
                        edge.start = topLeft + glm::vec2(0, 0);
                        edge.end = topLeft + glm::vec2(0, tileSize);
                        edge.poly1 = poly.id;
                        edge.poly2 = neighborId;
                        m_edges.push_back(edge);
                    }
                }
                
                if (y > 0 && m_pPathfindingManager->IsTileWalkable(x, y - 1))
                {
                    // Find polygon index for the neighbor above
                    int neighborId = (y - 1) * width + x;
                    if (neighborId < static_cast<int>(m_polygons.size()))
                    {
                        poly.neighbors.push_back(neighborId);
                        m_polygons[neighborId].neighbors.push_back(poly.id);
                        
                        // Create an edge
                        NavEdge edge;
                        edge.start = topLeft + glm::vec2(0, 0);
                        edge.end = topLeft + glm::vec2(tileSize, 0);
                        edge.poly1 = poly.id;
                        edge.poly2 = neighborId;
                        m_edges.push_back(edge);
                    }
                }
                
                m_polygons.push_back(poly);
            }
        }
    }
    
    // Add boundary edges (edges with only one polygon)
    for (const auto& poly : m_polygons)
    {
        for (size_t i = 0; i < poly.vertices.size(); i++)
        {
            size_t j = (i + 1) % poly.vertices.size();
            glm::vec2 start = poly.vertices[i];
            glm::vec2 end = poly.vertices[j];
            
            // Check if this edge already exists
            bool edgeExists = false;
            for (const auto& edge : m_edges)
            {
                if ((glm::distance2(edge.start, start) < 0.1f && glm::distance2(edge.end, end) < 0.1f) ||
                    (glm::distance2(edge.start, end) < 0.1f && glm::distance2(edge.end, start) < 0.1f))
                {
                    edgeExists = true;
                    break;
                }
            }
            
            if (!edgeExists)
            {
                NavEdge edge;
                edge.start = start;
                edge.end = end;
                edge.poly1 = poly.id;
                edge.poly2 = -1; // Boundary edge
                m_edges.push_back(edge);
            }
        }
    }
}

void NavMeshComponent::GenerateFromPolygons(const std::vector<NavPolygon>& polygons)
{
    m_polygons = polygons;
    m_edges.clear();
    
    // Generate edges from polygon neighbors
    for (const auto& poly : m_polygons)
    {
        for (int neighborId : poly.neighbors)
        {
            if (neighborId >= 0 && neighborId < static_cast<int>(m_polygons.size()) && neighborId > poly.id)
            {
                const auto& neighbor = m_polygons[neighborId];
                
                // Find shared vertices between polygons
                std::vector<glm::vec2> sharedVertices;
                for (const auto& v1 : poly.vertices)
                {
                    for (const auto& v2 : neighbor.vertices)
                    {
                        if (glm::distance2(v1, v2) < 0.1f)
                        {
                            sharedVertices.push_back(v1);
                        }
                    }
                }
                
                // If we found exactly 2 shared vertices, we have an edge
                if (sharedVertices.size() >= 2)
                {
                    NavEdge edge;
                    edge.start = sharedVertices[0];
                    edge.end = sharedVertices[1];
                    edge.poly1 = poly.id;
                    edge.poly2 = neighborId;
                    m_edges.push_back(edge);
                }
            }
        }
    }
}

std::vector<glm::vec2> NavMeshComponent::FindPath(const glm::vec2& start, const glm::vec2& goal)
{
    // Find the polygons containing start and goal
    int startPoly = FindPolygon(start);
    int goalPoly = FindPolygon(goal);
    
    // If we couldn't find valid polygons, get the nearest
    if (startPoly == -1)
    {
        glm::vec2 nearestPoint = GetNearestPointOnNavMesh(start);
        startPoly = FindPolygon(nearestPoint);
    }
    
    if (goalPoly == -1)
    {
        glm::vec2 nearestPoint = GetNearestPointOnNavMesh(goal);
        goalPoly = FindPolygon(nearestPoint);
    }
    
    // If we still can't find valid polygons, return empty path
    if (startPoly == -1 || goalPoly == -1)
    {
        return {};
    }
    
    // If start and goal are in the same polygon, just return a direct path
    if (startPoly == goalPoly)
    {
        // Check if there's a direct line of sight
        if (LineOfSight(start, goal))
        {
            return {start, goal};
        }
    }
    
    // Find a path through the polygons
    std::vector<int> polygonPath = FindPolygonPath(startPoly, goalPoly);
    if (polygonPath.empty())
    {
        return {};
    }
    
    // Use the funnel algorithm to find the actual path
    std::vector<glm::vec2> path = FunnelAlgorithm(polygonPath, start, goal);
    
    // Smooth the path
    return SmoothPath(path);
}

int NavMeshComponent::FindPolygon(const glm::vec2& point) const
{
    for (size_t i = 0; i < m_polygons.size(); i++)
    {
        if (IsPointInPolygon(point, m_polygons[i]))
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

glm::vec2 NavMeshComponent::GetNearestPointOnNavMesh(const glm::vec2& point) const
{
    float closestDist = std::numeric_limits<float>::max();
    glm::vec2 closestPoint = point;
    
    // Check all edges for the closest point
    for (const auto& edge : m_edges)
    {
        glm::vec2 projection = ProjectPointOnSegment(point, edge.start, edge.end);
        float dist = glm::distance2(point, projection);
        
        if (dist < closestDist)
        {
            closestDist = dist;
            closestPoint = projection;
        }
    }
    
    return closestPoint;
}

bool NavMeshComponent::IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const
{
    bool inside = false;
    
    // Ray casting algorithm
    for (size_t i = 0, j = polygon.vertices.size() - 1; i < polygon.vertices.size(); j = i++)
    {
        const glm::vec2& vi = polygon.vertices[i];
        const glm::vec2& vj = polygon.vertices[j];
        
        if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x))
        {
            inside = !inside;
        }
    }
    
    return inside;
}

std::vector<int> NavMeshComponent::FindPolygonPath(int startPoly, int endPoly) const
{


    // A* search for polygon path
    if (startPoly == -1 || endPoly == -1)
        return {};
        
    std::vector<int> cameFrom(m_polygons.size(), -1);
    std::vector<float> gScore(m_polygons.size(), std::numeric_limits<float>::max());
    std::vector<float> fScore(m_polygons.size(), std::numeric_limits<float>::max());
    
    gScore[startPoly] = 0.0f;
    fScore[startPoly] = glm::distance(m_polygons[startPoly].center, m_polygons[endPoly].center);
    
    // Custom comparator for priority queue
    auto compare = [&fScore](int a, int b) { return fScore[a] > fScore[b]; };
    std::priority_queue<int, std::vector<int>, decltype(compare)> openSet(compare);
    openSet.push(startPoly);
    
    std::vector<bool> inOpenSet(m_polygons.size(), false);
    inOpenSet[startPoly] = true;
    
    while (!openSet.empty())
    {
        int current = openSet.top();
        openSet.pop();
        inOpenSet[current] = false;
        
        if (current == endPoly)
        {
            // Reconstruct path
            std::vector<int> path;
            while (current != -1)
            {
                path.push_back(current);
                current = cameFrom[current];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        for (int neighbor : m_polygons[current].neighbors)
        {
            // Skip non-walkable polygons
            if (!m_polygons[neighbor].walkable)
                continue;

            if (neighbor < 0 || neighbor >= static_cast<int>(m_polygons.size()))
                continue;
                
            float tentativeGScore = gScore[current] + 
                                  glm::distance(m_polygons[current].center, m_polygons[neighbor].center);
                                  
            if (tentativeGScore < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeGScore;
                fScore[neighbor] = gScore[neighbor] + 
                                 glm::distance(m_polygons[neighbor].center, m_polygons[endPoly].center);
                                 
                if (!inOpenSet[neighbor])
                {
                    openSet.push(neighbor);
                    inOpenSet[neighbor] = true;
                }
            }
        }
    }
    
    return {}; // No path found
}

std::vector<glm::vec2> NavMeshComponent::SmoothPath(const std::vector<glm::vec2>& path) const
{
    if (path.size() <= 2)
        return path;
        
    std::vector<glm::vec2> smoothPath;
    smoothPath.push_back(path[0]);
    
    size_t current = 0;
    
    while (current < path.size() - 1)
    {
        size_t next = current + 1;
        
        // Try to find furthest point with line of sight
        for (size_t i = current + 2; i < path.size(); i++)
        {
            if (LineOfSight(path[current], path[i]))
            {
                next = i;
            }
        }
        
        current = next;
        smoothPath.push_back(path[current]);
    }
    
    return smoothPath;
}

float NavMeshComponent::DistancePointToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const
{
    glm::vec2 ab = b - a;
    glm::vec2 ap = p - a;
    
    float proj = glm::dot(ap, ab);
    float abLenSq = glm::dot(ab, ab);
    
    float d = proj / abLenSq;
    
    if (d <= 0.0f)
        return glm::length(ap);
    else if (d >= 1.0f)
        return glm::length(p - b);
    else
        return glm::length(p - (a + d * ab));
}

glm::vec2 NavMeshComponent::ProjectPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const
{
    glm::vec2 ab = b - a;
    glm::vec2 ap = p - a;
    
    float proj = glm::dot(ap, ab);
    float abLenSq = glm::dot(ab, ab);
    
    float d = proj / abLenSq;
    
    if (d <= 0.0f)
        return a;
    else if (d >= 1.0f)
        return b;
    else
        return a + d * ab;
}

bool NavMeshComponent::LineOfSight(const glm::vec2& start, const glm::vec2& end) const
{
    // Check if line segment intersects with any boundary edge
    for (const auto& edge : m_edges)
    {
        if (edge.poly2 == -1) // Boundary edge
        {
            // Check if line segments intersect
            glm::vec2 p1 = start;
            glm::vec2 p2 = end;
            glm::vec2 p3 = edge.start;
            glm::vec2 p4 = edge.end;
            
            glm::vec2 s1 = p2 - p1;
            glm::vec2 s2 = p4 - p3;
            
            float s = (-s1.y * (p1.x - p3.x) + s1.x * (p1.y - p3.y)) / 
                     (-s2.x * s1.y + s1.x * s2.y);
            float t = (s2.x * (p1.y - p3.y) - s2.y * (p1.x - p3.x)) / 
                     (-s2.x * s1.y + s1.x * s2.y);
            
            if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
            {
                return false; // Line of sight is blocked
            }
        }
    }
    
    return true; // No intersections found
}

void NavMeshComponent::DebugDraw() const
{
    GLShapesRenderer* renderer = GLShapesRenderer::GetInstance();
    if (!renderer)
        return;
    
    // Define colors for different elements
    glm::vec4 polyEdgeColor(0.0f, 1.0f, 0.0f, 0.6f);         // Softer green for polygon edges
    glm::vec4 polyFillColor(0.0f, 0.3f, 0.0f, 0.2f);         // Subtle green fill for walkable areas
    glm::vec4 nonWalkableEdgeColor(1.0f, 0.3f, 0.3f, 0.8f);  // Red edges for non-walkable areas
    glm::vec4 centerPointColor(0.8f, 0.1f, 0.1f, 0.6f);      // Softer red for center points
    glm::vec4 boundaryEdgeColor(0.9f, 0.2f, 0.2f, 0.7f);     // Softer red for boundary edges
    glm::vec4 connectionEdgeColor(0.2f, 0.5f, 0.9f, 0.5f);   // Softer blue for connection edges
    glm::vec4 entityOutlineColor(1.0f, 0.5f, 0.0f, 0.8f);    // Orange outline for entity projections
    
    // First draw only the walkable polygon fills
    for (const auto& poly : m_polygons)
    {
        // Only draw fill for walkable areas
        if (poly.walkable) {
            // Draw polygon fill (triangulate the polygon)
            for (size_t i = 1; i < poly.vertices.size() - 1; i++)
            {
                ColouredVertex2D v0 = {poly.vertices[0].x, poly.vertices[0].y,
                                      polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a};
                ColouredVertex2D v1 = {poly.vertices[i].x, poly.vertices[i].y,
                                      polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a};
                ColouredVertex2D v2 = {poly.vertices[i+1].x, poly.vertices[i+1].y,
                                      polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a};
                
                renderer->AddTriangle(v0, v1, v2);
            }
        }
    }
    
    // Draw NavMesh edges with different colors based on walkability
    for (const auto& poly : m_polygons)
    {
        // Choose edge color based on walkability
        glm::vec4 edgeColor = poly.walkable ? polyEdgeColor : nonWalkableEdgeColor;
        
        // Draw polygon edges
        for (size_t i = 0; i < poly.vertices.size(); i++)
        {
            size_t j = (i + 1) % poly.vertices.size();
            
            ColouredVertex2D v1 = {poly.vertices[i].x, poly.vertices[i].y,
                                  edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a};
            ColouredVertex2D v2 = {poly.vertices[j].x, poly.vertices[j].y,
                                  edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a};
            
            renderer->AddLine(v1, v2);
        }
        
        // Draw center point as small regular polygon (octagon) only for walkable areas
        if (poly.walkable) {
            ColouredVertex2D center = {poly.center.x, poly.center.y,
                                      centerPointColor.r, centerPointColor.g, centerPointColor.b, centerPointColor.a};
            renderer->AddRegularPolygon(center, 3.0f, 8, 0.0f);
        }
    }
    
    // Draw connection/boundary edges
    for (const auto& edge : m_edges)
    {
        glm::vec4 edgeColor = (edge.poly2 == -1) ? boundaryEdgeColor : connectionEdgeColor;
        
        ColouredVertex2D v1 = {edge.start.x, edge.start.y,
                              edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a};
        ColouredVertex2D v2 = {edge.end.x, edge.end.y,
                              edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a};
        
        renderer->AddLine(v1, v2);
    }
    
    // Draw entity outline projections
    for (const auto& obstacle : m_obstacles)
    {
        if (!obstacle) continue;
        
        wolf::Transform2D* transform = obstacle->GetComponent<wolf::Transform2D>();
        if (!transform) continue;
        
        glm::vec2 position = transform->GetGlobalPosition();
        
        // Check if entity has a collider we can use for more accurate visualization
        ColliderComponent* collider = obstacle->GetComponent<ColliderComponent>();
        if (collider)
        {
            // For each collider box, draw its outline
            for (const auto& box : collider->GetColliderBoxes())
            {
                // Draw outline using the rectangle coordinates
                ColouredVertex2D tl_outline = {box.m_left, box.m_top, 
                                             entityOutlineColor.r, entityOutlineColor.g, entityOutlineColor.b, entityOutlineColor.a};
                ColouredVertex2D tr_outline = {box.m_right, box.m_top, 
                                             entityOutlineColor.r, entityOutlineColor.g, entityOutlineColor.b, entityOutlineColor.a};
                ColouredVertex2D bl_outline = {box.m_left, box.m_bottom, 
                                             entityOutlineColor.r, entityOutlineColor.g, entityOutlineColor.b, entityOutlineColor.a};
                ColouredVertex2D br_outline = {box.m_right, box.m_bottom, 
                                             entityOutlineColor.r, entityOutlineColor.g, entityOutlineColor.b, entityOutlineColor.a};
                
                renderer->AddLine(tl_outline, tr_outline);
                renderer->AddLine(tr_outline, br_outline);
                renderer->AddLine(br_outline, bl_outline);
                renderer->AddLine(bl_outline, tl_outline);
            }
        }
        else
        {
            // Fallback to a simple circle outline if no collider is found
            float radius = 16.0f;
            
            // Draw just the outline
            ColouredVertex2D entityOutline = {position.x, position.y,
                                             entityOutlineColor.r, entityOutlineColor.g, entityOutlineColor.b, entityOutlineColor.a};
            renderer->AddRegularPolygon(entityOutline, radius, 16, 0.0f);
        }
    }
    
    // Render all the shapes
    renderer->RenderAndDeleteTriangles();
    renderer->RenderAndDeleteLines();
}


std::vector<glm::vec2> NavMeshComponent::FunnelAlgorithm(
    const std::vector<int>& corridorPolygons,
    const glm::vec2& start,
    const glm::vec2& end) const
{
    // TO DO:
    //create a proper funnel through the corridor
    
    std::vector<glm::vec2> path;
    path.push_back(start);
    
    // Add centers of corridor polygons as waypoints
    for (size_t i = 1; i < corridorPolygons.size() - 1; i++)
    {
        path.push_back(m_polygons[corridorPolygons[i]].center);
    }
    
    path.push_back(end);
    return path;
}

void NavMeshComponent::UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles)
{
    // Store obstacles for visualization
    m_obstacles = obstacles;
    
    // Clear any previous obstacle markings
    for (auto& poly : m_polygons)
    {
        poly.walkable = true;
    }
    
    // For each obstacle, mark affected polygons as non-walkable
    for (wolf::GameObject* obstacle : obstacles)
    {
        if (!obstacle) continue;
        
        wolf::Transform2D* transform = obstacle->GetComponent<wolf::Transform2D>();
        if (!transform) continue;
        
        glm::vec2 position = transform->GetGlobalPosition();
        
        // Get the entity's collision bounds if available
        ColliderComponent* collider = obstacle->GetComponent<ColliderComponent>();
        if (collider)
        {
            // For each collider box
            for (const auto& box : collider->GetColliderBoxes())
            {
                // Mark all polygons that overlap with this box as non-walkable
                for (size_t i = 0; i < m_polygons.size(); i++)
                {
                    // Quick AABB test first
                    bool mayOverlap = false;
                    for (const auto& vertex : m_polygons[i].vertices)
                    {
                        if (vertex.x >= box.m_left && vertex.x <= box.m_right &&
                            vertex.y >= box.m_bottom && vertex.y <= box.m_top)
                        {
                            mayOverlap = true;
                            break;
                        }
                    }
                    
                    if (mayOverlap || IsPointInPolygon(position, m_polygons[i]))
                    {
                        m_polygons[i].walkable = false;
                    }
                }
            }
        }
        else
        {
            // Fallback to simple position check if no collider is found
            int polyIndex = FindPolygon(position);
            if (polyIndex >= 0 && polyIndex < m_polygons.size())
            {
                m_polygons[polyIndex].walkable = false;
            }
        }
    }
    
    // Update the pathfinding graph connections
    // Remove connections to non-walkable polygons
    for (auto& poly : m_polygons)
    {
        if (!poly.walkable) continue;
        
        // Filter out connections to non-walkable polygons
        std::vector<int> walkableNeighbors;
        for (int neighborId : poly.neighbors)
        {
            if (neighborId >= 0 && neighborId < m_polygons.size() && m_polygons[neighborId].walkable)
            {
                walkableNeighbors.push_back(neighborId);
            }
        }
        
        poly.neighbors = walkableNeighbors;
    }
}