#include "NavMeshComponent.h"
#include <algorithm>
#include <queue>
#include <limits>
#include <unordered_set>
#include <glm/gtx/norm.hpp>
#include <GLShapesRenderer.h>
#include <W_Transform2D.h>

NavMeshComponent::NavMeshComponent() : m_debugDrawEnabled(false), m_pPathfindingManager(nullptr)
{
    // Pre-allocate memory for data structures to avoid reallocations
    m_polygons.reserve(1000);
    m_edges.reserve(2000);
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
    m_spatialHash.clear();
    
    const int width = labyrinthManager->GetWidth();
    const int height = labyrinthManager->GetHeight();
    const float tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    
    // Reserve space for expected number of elements to avoid reallocations
    m_polygons.reserve(width * height / 2);  // Assuming roughly half the tiles are walkable
    m_edges.reserve(width * height);
    
    // Positions are stored temporarily for quick lookups to avoid O(n) searches
    std::unordered_map<int, int> posToPolyIndex;
    posToPolyIndex.reserve(width * height / 2);
    
    // First pass: Create polygons for walkable tiles
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (m_pPathfindingManager->IsTileWalkable(x, y))
            {
                NavPolygon poly;
                poly.id = m_polygons.size();
                poly.walkable = true;
                poly.visible = true;
                
                // Calculate world coordinates for the polygon vertices
                glm::vec2 topLeft = labyrinthManager->GetWorldPosition(glm::ivec2(x, y));
                
                // Define polygon vertices (clockwise)
                poly.vertices = {
                    topLeft,
                    topLeft + glm::vec2(tileSize, 0),
                    topLeft + glm::vec2(tileSize, tileSize),
                    topLeft + glm::vec2(0, tileSize)
                };
                
                // Calculate center
                poly.center = topLeft + glm::vec2(tileSize / 2.0f, tileSize / 2.0f);
                
                // Add to spatial hash for quick polygon finding
                int cellX = static_cast<int>(poly.center.x / SPATIAL_CELL_SIZE);
                int cellY = static_cast<int>(poly.center.y / SPATIAL_CELL_SIZE);
                m_spatialHash[{cellX, cellY}].push_back(poly.id);
                
                // Store mapping from position to polygon index for quick neighbor lookup
                int posKey = y * width + x;
                posToPolyIndex[posKey] = poly.id;
                
                m_polygons.push_back(poly);
            }
        }
    }
    
    // Second pass: Connect neighbors and create edges
    static const std::pair<int, int> directions[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int currentPosKey = y * width + x;
            auto currentIt = posToPolyIndex.find(currentPosKey);
            
            // Skip if this tile doesn't have a polygon
            if (currentIt == posToPolyIndex.end())
                continue;
                
            int currentPolyId = currentIt->second;
            NavPolygon& currentPoly = m_polygons[currentPolyId];
            
            for (const auto& [dx, dy] : directions)
            {
                int nx = x + dx;
                int ny = y + dy;
                
                // Skip if out of bounds
                if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                    continue;
                    
                // Find neighbor polygon
                int neighborPosKey = ny * width + nx;
                auto neighborIt = posToPolyIndex.find(neighborPosKey);
                
                // Skip if neighbor tile doesn't have a polygon
                if (neighborIt == posToPolyIndex.end())
                    continue;
                    
                int neighborPolyId = neighborIt->second;
                
                // Add neighbor relationship (only if not already added)
                if (std::find(currentPoly.neighbors.begin(), currentPoly.neighbors.end(), neighborPolyId) == currentPoly.neighbors.end())
                {
                    currentPoly.neighbors.push_back(neighborPolyId);
                    
                    // Create edge
                    NavEdge edge;
                    glm::vec2 topLeft = labyrinthManager->GetWorldPosition(glm::ivec2(x, y));
                    
                    if (dx == -1) { // Left neighbor
                        edge.start = topLeft;
                        edge.end = topLeft + glm::vec2(0, tileSize);
                    } else if (dx == 1) { // Right neighbor
                        edge.start = topLeft + glm::vec2(tileSize, 0);
                        edge.end = topLeft + glm::vec2(tileSize, tileSize);
                    } else if (dy == -1) { // Bottom neighbor
                        edge.start = topLeft;
                        edge.end = topLeft + glm::vec2(tileSize, 0);
                    } else { // Top neighbor
                        edge.start = topLeft + glm::vec2(0, tileSize);
                        edge.end = topLeft + glm::vec2(tileSize, tileSize);
                    }
                    
                    edge.poly1 = currentPolyId;
                    edge.poly2 = neighborPolyId;
                    m_edges.push_back(edge);
                }
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
    m_spatialHash.clear();
    
    // Rebuild spatial hash
    for (const auto& poly : m_polygons)
    {
        int cellX = static_cast<int>(poly.center.x / SPATIAL_CELL_SIZE);
        int cellY = static_cast<int>(poly.center.y / SPATIAL_CELL_SIZE);
        m_spatialHash[{cellX, cellY}].push_back(poly.id);
    }
    
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
    // Find the polygons containing start and goal using spatial hash for faster lookup
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
    std::vector<glm::vec2> path = ImprovedFunnelAlgorithm(polygonPath, start, goal);
    
    // Smooth the path
    return SmoothPath(path);
}

int NavMeshComponent::FindPolygon(const glm::vec2& point) const
{
    // Use spatial hash for faster lookup
    int cellX = static_cast<int>(point.x / SPATIAL_CELL_SIZE);
    int cellY = static_cast<int>(point.y / SPATIAL_CELL_SIZE);
    
    // Check primary cell
    auto cellIt = m_spatialHash.find({cellX, cellY});
    if (cellIt != m_spatialHash.end())
    {
        for (int polyId : cellIt->second)
        {
            if (IsPointInPolygon(point, m_polygons[polyId]))
            {
                return polyId;
            }
        }
    }
    
    // Check neighboring cells (point might be near cell boundary)
    static const std::pair<int, int> neighbors[] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1, 0},           {1, 0},
        {-1, 1},  {0, 1},  {1, 1}
    };
    
    for (const auto& [dx, dy] : neighbors)
    {
        auto it = m_spatialHash.find({cellX + dx, cellY + dy});
        if (it != m_spatialHash.end())
        {
            for (int polyId : it->second)
            {
                if (IsPointInPolygon(point, m_polygons[polyId]))
                {
                    return polyId;
                }
            }
        }
    }
    
    return -1;
}

glm::vec2 NavMeshComponent::GetNearestPointOnNavMesh(const glm::vec2& point) const
{
    // Use spatial hash to find nearby polygons
    int cellX = static_cast<int>(point.x / SPATIAL_CELL_SIZE);
    int cellY = static_cast<int>(point.y / SPATIAL_CELL_SIZE);
    
    float closestDist = std::numeric_limits<float>::max();
    glm::vec2 closestPoint = point;
    
    // Search in current and neighboring cells
    const int searchRadius = 2;  // Increase if necessary
    
    for (int dy = -searchRadius; dy <= searchRadius; dy++)
    {
        for (int dx = -searchRadius; dx <= searchRadius; dx++)
        {
            auto it = m_spatialHash.find({cellX + dx, cellY + dy});
            if (it != m_spatialHash.end())
            {
                for (int polyId : it->second)
                {
                    const auto& poly = m_polygons[polyId];
                    
                    // Check if point is in polygon
                    if (IsPointInPolygon(point, poly))
                    {
                        return point;
                    }
                    
                    // Check edges
                    for (size_t i = 0; i < poly.vertices.size(); i++)
                    {
                        size_t j = (i + 1) % poly.vertices.size();
                        
                        glm::vec2 projection = ProjectPointOnSegment(point, poly.vertices[i], poly.vertices[j]);
                        float dist = glm::distance2(point, projection);
                        
                        if (dist < closestDist)
                        {
                            closestDist = dist;
                            closestPoint = projection;
                        }
                    }
                }
            }
        }
    }
    
    return closestPoint;
}

bool NavMeshComponent::IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const
{
    // Quick AABB check first (major optimization)
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& vertex : polygon.vertices)
    {
        minX = std::min(minX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxX = std::max(maxX, vertex.x);
        maxY = std::max(maxY, vertex.y);
    }
    
    if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY)
    {
        return false;
    }
    
    // Ray casting algorithm
    bool inside = false;
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
    // Early exit for invalid input
    if (startPoly == -1 || endPoly == -1 || startPoly >= m_polygons.size() || endPoly >= m_polygons.size())
        return {};
    
    // A* search for polygon path
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
    
    // Use a closed set to avoid revisiting nodes
    std::vector<bool> closedSet(m_polygons.size(), false);
    
    while (!openSet.empty())
    {
        int current = openSet.top();
        openSet.pop();
        inOpenSet[current] = false;
        
        // Add to closed set
        closedSet[current] = true;
        
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
            // Skip invalid or non-walkable polygons
            if (neighbor < 0 || neighbor >= static_cast<int>(m_polygons.size()) || 
                !m_polygons[neighbor].walkable || closedSet[neighbor])
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
    smoothPath.reserve(path.size());  // Pre-allocate memory
    smoothPath.push_back(path[0]);
    
    size_t current = 0;
    
    while (current < path.size() - 1)
    {
        size_t next = current + 1;
        
        // Try to find furthest point with line of sight
        for (size_t i = path.size() - 1; i > current + 1; i--)
        {
            if (LineOfSight(path[current], path[i]))
            {
                next = i;
                break;
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
    // Implementation of fast ray-segment intersection
    // Check if line segment intersects with any boundary edge
    glm::vec2 rayVector = end - start;
    
    for (const auto& edge : m_edges)
    {
        if (edge.poly2 == -1) // Boundary edge
        {
            glm::vec2 v1 = edge.start;
            glm::vec2 v2 = edge.end;
            
            // Line-line intersection test using parametric equation
            glm::vec2 edgeVector = v2 - v1;
            float crossProduct = rayVector.x * edgeVector.y - rayVector.y * edgeVector.x;
            
            // Skip parallel lines
            if (std::abs(crossProduct) < 0.001f)
                continue;
                
            glm::vec2 startToV1 = v1 - start;
            float t = (startToV1.x * edgeVector.y - startToV1.y * edgeVector.x) / crossProduct;
            float u = (startToV1.x * rayVector.y - startToV1.y * rayVector.x) / crossProduct;
            
            if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
                return false; // Intersection found, line of sight is blocked
        }
    }
    
    return true; // No intersections found
}

void NavMeshComponent::DebugDraw() const
{
    // Early exit conditions
    if (!m_debugDrawEnabled)
        return;
        
    GLShapesRenderer* renderer = GLShapesRenderer::GetInstance();
    if (!renderer)
        return;
    
    // Define colors for different elements (only once)
    static const glm::vec4 polyFillColor(0.0f, 0.3f, 0.0f, 0.2f);     // Subtle green fill for walkable areas
    static const glm::vec4 polyEdgeColor(0.0f, 1.0f, 0.0f, 0.6f);     // Softer green for polygon edges
    static const glm::vec4 nonWalkableEdgeColor(1.0f, 0.3f, 0.3f, 0.8f); // Red edges for non-walkable areas
    static const glm::vec4 boundaryEdgeColor(0.9f, 0.2f, 0.2f, 0.7f); // Red for boundary edges
    static const glm::vec4 connectionEdgeColor(0.2f, 0.5f, 0.9f, 0.5f); // Blue for connection edges
    static const glm::vec4 entityOutlineColor(1.0f, 0.5f, 0.0f, 0.8f); // Orange for entity outlines
    static const glm::vec4 playerOutlineColor(0.0f, 0.8f, 1.0f, 0.8f); // Blue for player outline
    
    // Pre-allocate vertex collections to avoid repeated memory allocations
    const size_t estimatedTriangles = m_polygons.size() * 2; // Estimate 2 triangles per polygon
    const size_t estimatedLines = m_edges.size() + m_polygons.size() * 4 + m_obstacles.size() * 4;
    
    // Precalculate properties for rendering
    const bool hasObstacles = !m_obstacles.empty();
    const bool hasPlayer = hasObstacles && m_pPathfindingManager && m_pPathfindingManager->GetLabyrinthManager() && 
                           m_pPathfindingManager->GetLabyrinthManager()->GetPlayer();
    wolf::GameObject* player = hasPlayer ? m_pPathfindingManager->GetLabyrinthManager()->GetPlayer() : nullptr;
    
    // ======= PHASE 1: FILL POLYGONS ========
    // Only draw polygons that are visible and walkable
    for (const auto& poly : m_polygons)
    {
        if (!poly.visible || !poly.walkable) 
            continue;
            
        const size_t vertCount = poly.vertices.size();
        if (vertCount < 3) 
            continue;
            
        // Triangle fan optimization - all triangles share the first vertex
        const glm::vec2& v0 = poly.vertices[0];
        ColouredVertex2D firstVertex = {v0.x, v0.y, polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a};
        
        for (size_t i = 1; i < vertCount - 1; i++)
        {
            const glm::vec2& v1 = poly.vertices[i];
            const glm::vec2& v2 = poly.vertices[i+1];
            
            renderer->AddTriangle(
                firstVertex,
                {v1.x, v1.y, polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a},
                {v2.x, v2.y, polyFillColor.r, polyFillColor.g, polyFillColor.b, polyFillColor.a}
            );
        }
    }
    
    // Batch render all triangles at once
    renderer->RenderAndDeleteTriangles();
    
    // ======= PHASE 2: DRAW EDGES ========
    // Draw visible polygon edges
    for (const auto& poly : m_polygons)
    {
        if (!poly.visible) 
            continue;
            
        // Choose edge color based on walkability (reuse color)
        const glm::vec4& edgeColor = poly.walkable ? polyEdgeColor : nonWalkableEdgeColor;
        
        const size_t vertCount = poly.vertices.size();
        for (size_t i = 0; i < vertCount; i++)
        {
            const glm::vec2& v1 = poly.vertices[i];
            const glm::vec2& v2 = poly.vertices[(i + 1) % vertCount];
            
            renderer->AddLine(
                {v1.x, v1.y, edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a},
                {v2.x, v2.y, edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a}
            );
        }
    }
    
    // Draw connection/boundary edges (only those with visible polygons)
    for (const auto& edge : m_edges)
    {
        // Skip edges if either polygon is invisible (culling optimization)
        if ((edge.poly1 >= 0 && edge.poly1 < m_polygons.size() && !m_polygons[edge.poly1].visible) ||
            (edge.poly2 >= 0 && edge.poly2 < m_polygons.size() && !m_polygons[edge.poly2].visible))
            continue;
        
        // Reuse colors
        const glm::vec4& edgeColor = (edge.poly2 == -1) ? boundaryEdgeColor : connectionEdgeColor;
        
        renderer->AddLine(
            {edge.start.x, edge.start.y, edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a},
            {edge.end.x, edge.end.y, edgeColor.r, edgeColor.g, edgeColor.b, edgeColor.a}
        );
    }
    
    // ======= PHASE 3: DRAW OBSTACLES ========
    // Only process if we have obstacles
    if (hasObstacles)
    {
        static const float obstacleRadius = 16.0f; // Default radius for visualization
        
        for (const auto& obstacle : m_obstacles)
        {
            if (!obstacle) 
                continue;
            
            wolf::Transform2D* transform = obstacle->GetComponent<wolf::Transform2D>();
            if (!transform) 
                continue;
            
            glm::vec2 position = transform->GetGlobalPosition();
            
            // Determine if this is the player (only check once)
            bool isPlayer = (obstacle == player);
            const glm::vec4& outlineColor = isPlayer ? playerOutlineColor : entityOutlineColor;
            
            // Draw a simple circle outline (with fewer segments for better performance)
            renderer->AddRegularPolygon(
                {position.x, position.y, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a},
                obstacleRadius, 8, 0.0f  // Reduced from 16 segments to 8
            );
        }
    }
    
    // Batch render all lines at once
    renderer->RenderAndDeleteLines();
}

void NavMeshComponent::UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles)
{
    // Store obstacles for visualization
    m_obstacles = obstacles;
    
    // Quick exit for empty obstacle list
    if (obstacles.empty())
    {
        // Only process if there were previously affected polygons
        if (!m_obstacleAffectedPolygons.empty())
        {
            // Use direct array indexing for better performance
            const size_t affectedCount = m_obstacleAffectedPolygons.size();
            for (size_t i = 0; i < affectedCount; i++)
            {
                int polyId = m_obstacleAffectedPolygons[i];
                if (polyId >= 0 && polyId < static_cast<int>(m_polygons.size()))
                {
                    m_polygons[polyId].walkable = true;
                    
                    // Restore visibility if it was changed
                    auto it = m_originalVisibility.find(polyId);
                    if (it != m_originalVisibility.end())
                    {
                        m_polygons[polyId].visible = it->second;
                    }
                }
            }
            
            m_obstacleAffectedPolygons.clear();
            m_originalVisibility.clear();
        }
        return;
    }
    
    // Clear previous obstacle markings but only for affected polygons
    const size_t affectedCount = m_obstacleAffectedPolygons.size();
    for (size_t i = 0; i < affectedCount; i++)
    {
        int polyId = m_obstacleAffectedPolygons[i];
        if (polyId >= 0 && polyId < static_cast<int>(m_polygons.size()))
        {
            m_polygons[polyId].walkable = true;
            
            // Restore visibility if it was changed
            auto it = m_originalVisibility.find(polyId);
            if (it != m_originalVisibility.end())
            {
                m_polygons[polyId].visible = it->second;
            }
        }
    }
    
    m_obstacleAffectedPolygons.clear();
    m_originalVisibility.clear();
    
    // Reserve space to avoid reallocations
    std::unordered_set<int> newlyAffectedPolygons;
    newlyAffectedPolygons.reserve(obstacles.size() * 4); // Estimate 4 polygons per obstacle
    
    // Cache player for faster comparisons
    wolf::GameObject* player = nullptr;
    if (m_pPathfindingManager && m_pPathfindingManager->GetLabyrinthManager())
    {
        player = m_pPathfindingManager->GetLabyrinthManager()->GetPlayer();
    }
    
    // Optimization: Pre-check if spatial hash exists and SPATIAL_CELL_SIZE is valid
    const bool useSpatialHash = !m_spatialHash.empty() && SPATIAL_CELL_SIZE > 0;
    
    // Process each obstacle using only Transform position (no colliders)
    const size_t obstacleCount = obstacles.size();
    for (size_t i = 0; i < obstacleCount; i++)
    {
        wolf::GameObject* obstacle = obstacles[i];
        if (!obstacle) continue;
        
        wolf::Transform2D* transform = obstacle->GetComponent<wolf::Transform2D>();
        if (!transform) continue;
        
        const glm::vec2 position = transform->GetGlobalPosition();
        const bool isPlayer = (obstacle == player);
        
        // Use a smaller radius for optimization - just enough to cover typical entity size
        const float obstacleRadius = 24.0f;
        const float radiusSq = obstacleRadius * obstacleRadius; // Pre-square for distance2 comparisons
        
        // Process using spatial hash if available (much faster for large worlds)
        if (useSpatialHash)
        {
            // Calculate bounding box in cell coordinates
            const int minCellX = static_cast<int>((position.x - obstacleRadius) / SPATIAL_CELL_SIZE);
            const int minCellY = static_cast<int>((position.y - obstacleRadius) / SPATIAL_CELL_SIZE);
            const int maxCellX = static_cast<int>((position.x + obstacleRadius) / SPATIAL_CELL_SIZE);
            const int maxCellY = static_cast<int>((position.y + obstacleRadius) / SPATIAL_CELL_SIZE);
            
            // Process cells in bounding box
            for (int cellY = minCellY; cellY <= maxCellY; cellY++)
            {
                for (int cellX = minCellX; cellX <= maxCellX; cellX++)
                {
                    auto cellIt = m_spatialHash.find({cellX, cellY});
                    if (cellIt != m_spatialHash.end())
                    {
                        const std::vector<int>& cellPolygons = cellIt->second;
                        const size_t polyCount = cellPolygons.size();
                        
                        for (size_t j = 0; j < polyCount; j++)
                        {
                            const int polyId = cellPolygons[j];
                            if (polyId < 0 || polyId >= static_cast<int>(m_polygons.size()))
                                continue;
                                
                            // Skip if already processed
                            if (newlyAffectedPolygons.count(polyId) > 0)
                                continue;
                                
                            // Get reference to the polygon for more efficient access
                            const NavPolygon& poly = m_polygons[polyId];
                            
                            // First check: containment test (position inside polygon)
                            if (IsPointInPolygon(position, poly))
                            {
                                ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                                continue;
                            }
                            
                            // Second check: center point distance (fastest check)
                            if (glm::distance2(poly.center, position) <= radiusSq)
                            {
                                ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                                continue;
                            }
                            
                            // Last check: vertices distance (more expensive, do last)
                            bool affected = false;
                            const size_t vertCount = poly.vertices.size();
                            for (size_t k = 0; k < vertCount; k++)
                            {
                                if (glm::distance2(poly.vertices[k], position) <= radiusSq)
                                {
                                    affected = true;
                                    break;
                                }
                            }
                            
                            if (affected)
                            {
                                ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // Fallback to linear scan (slower but works without spatial hash)
            const size_t polyCount = m_polygons.size();
            for (size_t j = 0; j < polyCount; j++)
            {
                const int polyId = static_cast<int>(j);
                
                // Skip if already processed
                if (newlyAffectedPolygons.count(polyId) > 0)
                    continue;
                    
                // Get reference to the polygon for more efficient access
                const NavPolygon& poly = m_polygons[polyId];
                
                // First check: containment test (position inside polygon)
                if (IsPointInPolygon(position, poly))
                {
                    ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                    continue;
                }
                
                // Second check: center point distance (fastest check)
                if (glm::distance2(poly.center, position) <= radiusSq)
                {
                    ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                    continue;
                }
                
                // Last check: vertices distance (more expensive, do last)
                bool affected = false;
                const size_t vertCount = poly.vertices.size();
                for (size_t k = 0; k < vertCount; k++)
                {
                    if (glm::distance2(poly.vertices[k], position) <= radiusSq)
                    {
                        affected = true;
                        break;
                    }
                }
                
                if (affected)
                {
                    ProcessAffectedPolygon(polyId, isPlayer, newlyAffectedPolygons);
                }
            }
        }
    }
    
    // Store affected polygons for next update
    m_obstacleAffectedPolygons.assign(newlyAffectedPolygons.begin(), newlyAffectedPolygons.end());
}

inline void NavMeshComponent::ProcessAffectedPolygon(int polyId, bool isPlayer, std::unordered_set<int>& affectedPolygons)
{
    if (isPlayer)
    {
        // Save original visibility for later restoration
        if (m_originalVisibility.find(polyId) == m_originalVisibility.end())
        {
            m_originalVisibility[polyId] = m_polygons[polyId].visible;
        }
        
        // Make polygon invisible for player but still walkable
        m_polygons[polyId].visible = false;
        m_polygons[polyId].walkable = true;
    }
    else
    {
        // Make polygon non-walkable for other entities
        m_polygons[polyId].walkable = false;
    }
    
    affectedPolygons.insert(polyId);
}

std::vector<glm::vec2> NavMeshComponent::ImprovedFunnelAlgorithm(
    const std::vector<int>& corridorPolygons,
    const glm::vec2& start,
    const glm::vec2& end) const
{
    if (corridorPolygons.empty())
        return {start, end};
    
    if (corridorPolygons.size() == 1)
        return {start, end};
    
    // Identify portals (edges) between consecutive polygons
    std::vector<std::pair<glm::vec2, glm::vec2>> portals;
    portals.reserve(corridorPolygons.size());
    
    // Add start point as first portal
    portals.push_back({start, start});
    
    // Find shared edges between consecutive polygons
    for (size_t i = 0; i < corridorPolygons.size() - 1; i++)
    {
        int current = corridorPolygons[i];
        int next = corridorPolygons[i + 1];
        
        // Find the shared edge
        for (const auto& edge : m_edges)
        {
            if ((edge.poly1 == current && edge.poly2 == next) ||
                (edge.poly1 == next && edge.poly2 == current))
            {
                portals.push_back({edge.start, edge.end});
                break;
            }
        }
    }
    
    // Add end point as last portal
    portals.push_back({end, end});
    
    // Simple funnel algorithm implementation
    std::vector<glm::vec2> path;
    path.push_back(start);
    
    glm::vec2 apex = start;
    glm::vec2 leftLeg = start;
    glm::vec2 rightLeg = start;
    
    int apexIndex = 0;
    int leftIndex = 0;
    int rightIndex = 0;
    
    // Process each portal
    for (size_t i = 1; i < portals.size(); i++)
    {
        const auto& portal = portals[i];
        glm::vec2 left = portal.first;
        glm::vec2 right = portal.second;
        
        // Update right leg
        bool rightUpdated = false;
        {
            // Compute cross product of current right leg
            glm::vec2 newRightLeg = right - apex;
            glm::vec2 oldRightLeg = rightLeg - apex;
            
            float cross = oldRightLeg.x * newRightLeg.y - oldRightLeg.y * newRightLeg.x;
            
            if (cross < 0.0f)  // New leg is further right
            {
                rightLeg = right;
                rightIndex = i;
                rightUpdated = true;
            }
        }
        
        // Update left leg
        bool leftUpdated = false;
        {
            // Compute cross product of current left leg
            glm::vec2 newLeftLeg = left - apex;
            glm::vec2 oldLeftLeg = leftLeg - apex;
            
            float cross = oldLeftLeg.x * newLeftLeg.y - oldLeftLeg.y * newLeftLeg.x;
            
            if (cross > 0.0f)  // New leg is further left
            {
                leftLeg = left;
                leftIndex = i;
                leftUpdated = true;
            }
        }
        
        // Check if we need to narrow the funnel by moving the apex
        if (rightUpdated)
        {
            // Check if right leg crosses over left leg
            glm::vec2 rightToLeft = leftLeg - rightLeg;
            glm::vec2 rightToApex = apex - rightLeg;
            
            float cross = rightToApex.x * rightToLeft.y - rightToApex.y * rightToLeft.x;
            
            if (cross < 0.0f)  // Right leg crosses left leg, move apex to left leg
            {
                apex = leftLeg;
                apexIndex = leftIndex;
                
                // Add new apex to path
                path.push_back(apex);
                
                // Reset funnel to new apex
                leftLeg = apex;
                rightLeg = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                
                // Restart funnel algorithm from new apex
                i = apexIndex;
                continue;
            }
        }
        
        if (leftUpdated)
        {
            // Check if left leg crosses over right leg
            glm::vec2 leftToRight = rightLeg - leftLeg;
            glm::vec2 leftToApex = apex - leftLeg;
            
            float cross = leftToApex.x * leftToRight.y - leftToApex.y * leftToRight.x;
            
            if (cross < 0.0f)  // Left leg crosses right leg, move apex to right leg
            {
                apex = rightLeg;
                apexIndex = rightIndex;
                
                // Add new apex to path
                path.push_back(apex);
                
                // Reset funnel to new apex
                leftLeg = apex;
                rightLeg = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;
                
                // Restart funnel algorithm from new apex
                i = apexIndex;
                continue;
            }
        }
    }
    
    // Add end point
    if (path.back() != end)
    {
        path.push_back(end);
    }
    
    return path;
}

bool NavMeshComponent::IsPathValid(const std::vector<glm::vec2>& path) const
{
    if (path.size() < 2)
        return false;
        
    // Check each segment of the path
    for (size_t i = 0; i < path.size() - 1; i++)
    {
        // Check if line of sight between points
        if (!LineOfSight(path[i], path[i+1]))
            return false;
            
        // Check if points are too close to obstacle edges
        if (i > 0) { // Skip start point
            float minDistToEdge = std::numeric_limits<float>::max();
            
            // Find minimum distance to any boundary edge
            for (const auto& edge : m_edges)
            {
                if (edge.poly2 == -1) // Boundary edge
                {
                    float dist = DistancePointToSegment(path[i], edge.start, edge.end);
                    minDistToEdge = std::min(minDistToEdge, dist);
                }
            }
            
            // If too close to an edge, path might be invalid
            if (minDistToEdge < 10.0f) // Adjust threshold as needed
                return false;
        }
    }
    
    return true;
}

std::vector<glm::vec2> NavMeshComponent::CreateGridBasedPath(const glm::vec2& start, const glm::vec2& goal)
{
    // Find tile size from LabyrinthManager
    const float tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    
    // Convert world positions to grid positions
    glm::ivec2 startTile = glm::ivec2(start) / static_cast<int>(tileSize);
    glm::ivec2 goalTile = glm::ivec2(goal) / static_cast<int>(tileSize);
    
    // Ask grid-based pathfinder to find path
    std::vector<glm::ivec2> gridPath;
    if (m_pPathfindingManager)
    {
        gridPath = m_pPathfindingManager->FindPath(startTile, goalTile);
    }
    
    // Convert grid path back to world coordinates
    std::vector<glm::vec2> worldPath;
    for (const auto& tile : gridPath)
    {
        worldPath.push_back(glm::vec2(tile) * tileSize + glm::vec2(tileSize/2.0f));
    }
    
    return worldPath;
}