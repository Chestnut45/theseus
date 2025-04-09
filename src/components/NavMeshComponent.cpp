#include "NavMeshComponent.h"
#include <algorithm>
#include <queue>
#include <limits>
#include <unordered_set>
#include <glm/gtx/norm.hpp>
#include <GLShapesRenderer.h>

NavMeshComponent::NavMeshComponent() : m_debugDrawEnabled(false), m_pPathfindingManager(nullptr)
{
    m_polygons.reserve(1000);
    m_edges.reserve(2000);
}

NavMeshComponent::~NavMeshComponent()
{
}

void NavMeshComponent::Update(float delta)
{
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_7))
        m_debugDrawEnabled = !m_debugDrawEnabled;
}

void NavMeshComponent::Init(PathfindingManager* pathfindingManager)
{
    m_pPathfindingManager = pathfindingManager;
}

void NavMeshComponent::GenerateFromLabyrinth(LabyrinthManager* labyrinthManager)
{
    m_polygons.clear();
    m_edges.clear();
    m_spatialHash.clear();
    
    const int width = labyrinthManager->GetWidth();
    const int height = labyrinthManager->GetHeight();
    const float tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    
    std::unordered_map<int, int> posToPolyIndex;
    
    // Create polygons for walkable tiles
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (!m_pPathfindingManager->IsTileWalkable(x, y))
                continue;
                
            // Create polygon
            glm::vec2 topLeft = labyrinthManager->GetWorldPosition(glm::ivec2(x, y));
            int polyId = m_polygons.size();
            
            NavPolygon poly;
            poly.id = polyId;
            poly.walkable = poly.visible = true;
            poly.vertices = {
                topLeft,
                topLeft + glm::vec2(tileSize, 0),
                topLeft + glm::vec2(tileSize, tileSize),
                topLeft + glm::vec2(0, tileSize)
            };
            poly.center = topLeft + glm::vec2(tileSize / 2.0f);
            
            // Add to spatial hash
            m_spatialHash[{static_cast<int>(poly.center.x / SPATIAL_CELL_SIZE), 
                           static_cast<int>(poly.center.y / SPATIAL_CELL_SIZE)}].push_back(polyId);
            
            posToPolyIndex[y * width + x] = polyId;
            m_polygons.push_back(poly);
        }
    }
    
    // Connect neighbors
    static const std::pair<int, int> directions[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            auto currentIt = posToPolyIndex.find(y * width + x);
            if (currentIt == posToPolyIndex.end())
                continue;
                
            int currentPolyId = currentIt->second;
            NavPolygon& currentPoly = m_polygons[currentPolyId];
            
            for (const auto& [dx, dy] : directions)
            {
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                    continue;
                    
                auto neighborIt = posToPolyIndex.find(ny * width + nx);
                if (neighborIt == posToPolyIndex.end())
                    continue;
                    
                int neighborPolyId = neighborIt->second;
                
                if (std::find(currentPoly.neighbors.begin(), currentPoly.neighbors.end(), neighborPolyId) != currentPoly.neighbors.end())
                    continue;

                currentPoly.neighbors.push_back(neighborPolyId);
                
                // Create edge
                NavEdge edge;
                glm::vec2 topLeft = labyrinthManager->GetWorldPosition(glm::ivec2(x, y));
                
                if (dx == -1) // Left
                    edge = {topLeft, topLeft + glm::vec2(0, tileSize), currentPolyId, neighborPolyId};
                else if (dx == 1) // Right
                    edge = {topLeft + glm::vec2(tileSize, 0), topLeft + glm::vec2(tileSize, tileSize), currentPolyId, neighborPolyId};
                else if (dy == -1) // Bottom
                    edge = {topLeft, topLeft + glm::vec2(tileSize, 0), currentPolyId, neighborPolyId};
                else // Top
                    edge = {topLeft + glm::vec2(0, tileSize), topLeft + glm::vec2(tileSize, tileSize), currentPolyId, neighborPolyId};
                
                m_edges.push_back(edge);
            }
        }
    }
    
    // Add boundary edges
    for (const auto& poly : m_polygons)
    {
        for (size_t i = 0; i < poly.vertices.size(); i++)
        {
            size_t j = (i + 1) % poly.vertices.size();
            glm::vec2 start = poly.vertices[i], end = poly.vertices[j];
            
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
                m_edges.push_back({start, end, poly.id, -1});
        }
    }
}

std::vector<glm::vec2> NavMeshComponent::FindPath(const glm::vec2& start, const glm::vec2& goal)
{
    // Find containing polygons
    int startPoly = FindPolygon(start);
    int goalPoly = FindPolygon(goal);
    
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
    
    if (startPoly == -1 || goalPoly == -1)
        return {};
    
    // Direct path if possible
    if (startPoly == goalPoly && LineOfSight(start, goal))
        return {start, goal};
    
    // Find polygon corridor
    std::vector<int> polyPath = FindPolygonPath(startPoly, goalPoly);
    if (polyPath.empty())
        return {};
    
    // Find actual path through corridor
    std::vector<glm::vec2> path = ImprovedFunnelAlgorithm(polyPath, start, goal);
    return SmoothPath(path);
}

int NavMeshComponent::FindPolygon(const glm::vec2& point) const 
{
    int cellX = static_cast<int>(point.x / SPATIAL_CELL_SIZE);
    int cellY = static_cast<int>(point.y / SPATIAL_CELL_SIZE);
    
    // Check primary and adjacent cells
    static const std::pair<int, int> cellOffsets[] = {
        {0,0}, {-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}
    };
    
    for (const auto& [dx, dy] : cellOffsets) 
    {
        auto it = m_spatialHash.find({cellX + dx, cellY + dy});
        if (it == m_spatialHash.end())
            continue;
            
        for (int polyId : it->second)
            if (IsPointInPolygon(point, m_polygons[polyId]))
                return polyId;
    }
    
    return -1;
}

bool NavMeshComponent::IsPointInPolygon(const glm::vec2& point, const NavPolygon& polygon) const
{
    // Quick AABB check
    float minX = std::numeric_limits<float>::max(), minY = minX;
    float maxX = -minX, maxY = -minY;
    
    for (const auto& vertex : polygon.vertices)
    {
        minX = std::min(minX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxX = std::max(maxX, vertex.x);
        maxY = std::max(maxY, vertex.y);
    }
    
    if (point.x < minX || point.x > maxX || point.y < minY || point.y > maxY)
        return false;
    
    // Ray casting
    bool inside = false;
    for (size_t i = 0, j = polygon.vertices.size() - 1; i < polygon.vertices.size(); j = i++)
    {
        const auto& vi = polygon.vertices[i];
        const auto& vj = polygon.vertices[j];
        
        if (((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x))
            inside = !inside;
    }
    
    return inside;
}

glm::vec2 NavMeshComponent::ProjectPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const 
{
    glm::vec2 ab = b - a;
    glm::vec2 ap = p - a;
    
    float proj = glm::dot(ap, ab);
    float abLenSq = glm::dot(ab, ab);
    
    float d = abLenSq > 0 ? proj / abLenSq : 0;
    
    if (d <= 0.0f) return a;
    if (d >= 1.0f) return b;
    return a + d * ab;
}

float NavMeshComponent::DistancePointToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) const 
{
    return glm::length(p - ProjectPointOnSegment(p, a, b));
}

bool NavMeshComponent::LineOfSight(const glm::vec2& start, const glm::vec2& end) const 
{
    glm::vec2 ray = end - start;
    
    for (const auto& edge : m_edges) 
    {
        if (edge.poly2 == -1) // Only boundary edges block sight
        { 
            glm::vec2 edgeVec = edge.end - edge.start;
            float cross = ray.x * edgeVec.y - ray.y * edgeVec.x;
            
            if (std::abs(cross) < 0.001f) // Parallel
                continue;
                
            glm::vec2 s2e = edge.start - start;
            float t = (s2e.x * edgeVec.y - s2e.y * edgeVec.x) / cross;
            float u = (s2e.x * ray.y - s2e.y * ray.x) / cross;
            
            if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
                return false; // Blocked
        }
    }
    
    return true;
}

std::vector<int> NavMeshComponent::FindPolygonPath(int startPoly, int endPoly) const
{
    if (startPoly == -1 || endPoly == -1)
        return {};
    
    const size_t polyCount = m_polygons.size();
    std::vector<int> cameFrom(polyCount, -1);
    std::vector<float> gScore(polyCount, std::numeric_limits<float>::max());
    std::vector<float> fScore(polyCount, std::numeric_limits<float>::max());
    std::vector<bool> closed(polyCount, false);
    
    auto compare = [&fScore](int a, int b) { return fScore[a] > fScore[b]; };
    std::priority_queue<int, std::vector<int>, decltype(compare)> openSet(compare);
    
    gScore[startPoly] = 0;
    fScore[startPoly] = glm::distance(m_polygons[startPoly].center, m_polygons[endPoly].center);
    openSet.push(startPoly);
    
    while (!openSet.empty())
    {
        int current = openSet.top();
        openSet.pop();
        
        if (closed[current]) continue;
        closed[current] = true;
        
        if (current == endPoly) 
        {
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
            if (neighbor < 0 || neighbor >= polyCount || !m_polygons[neighbor].walkable || closed[neighbor])
                continue;
                
            float tentative = gScore[current] + glm::distance(m_polygons[current].center, m_polygons[neighbor].center);
            
            if (tentative < gScore[neighbor])
            {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative;
                fScore[neighbor] = tentative + glm::distance(m_polygons[neighbor].center, m_polygons[endPoly].center);
                openSet.push(neighbor);
            }
        }
    }
    
    return {};
}

std::vector<glm::vec2> NavMeshComponent::SmoothPath(const std::vector<glm::vec2>& path) const
{
    if (path.size() <= 2)
        return path;
        
    std::vector<glm::vec2> result;
    result.push_back(path[0]);
    
    size_t current = 0;
    while (current < path.size() - 1)
    {
        for (size_t i = path.size() - 1; i > current; i--)
        {
            if (LineOfSight(path[current], path[i]))
            {
                current = i;
                break;
            }
        }
        result.push_back(path[current]);
    }
    
    return result;
}

glm::vec2 NavMeshComponent::GetNearestPointOnNavMesh(const glm::vec2& point) const
{
    int cellX = static_cast<int>(point.x / SPATIAL_CELL_SIZE);
    int cellY = static_cast<int>(point.y / SPATIAL_CELL_SIZE);
    
    float bestDist = std::numeric_limits<float>::max();
    glm::vec2 bestPoint = point;
    
    for (int dy = -2; dy <= 2; dy++) 
    {
        for (int dx = -2; dx <= 2; dx++) 
        {
            auto it = m_spatialHash.find({cellX + dx, cellY + dy});
            if (it == m_spatialHash.end())
                continue;
            
            for (int polyId : it->second) 
            {
                const auto& poly = m_polygons[polyId];
                
                if (IsPointInPolygon(point, poly))
                    return point;
                
                for (size_t i = 0; i < poly.vertices.size(); i++) 
                {
                    size_t j = (i + 1) % poly.vertices.size();
                    glm::vec2 proj = ProjectPointOnSegment(point, poly.vertices[i], poly.vertices[j]);
                    float dist = glm::distance2(point, proj);
                    
                    if (dist < bestDist) 
                    {
                        bestDist = dist;
                        bestPoint = proj;
                    }
                }
            }
        }
    }
    
    return bestPoint;
}

std::vector<glm::vec2> NavMeshComponent::ImprovedFunnelAlgorithm(
    const std::vector<int>& corridor, const glm::vec2& start, const glm::vec2& end) const 
{
    if (corridor.size() <= 1)
        return {start, end};
        
    // Build portals
    std::vector<std::pair<glm::vec2, glm::vec2>> portals;
    portals.push_back({start, start});
    
    for (size_t i = 0; i < corridor.size() - 1; i++) 
    {
        for (const auto& edge : m_edges) 
        {
            if ((edge.poly1 == corridor[i] && edge.poly2 == corridor[i+1]) ||
                (edge.poly1 == corridor[i+1] && edge.poly2 == corridor[i])) 
            {
                portals.push_back({edge.start, edge.end});
                break;
            }
        }
    }
    portals.push_back({end, end});
    
    // Apply funnel algorithm
    std::vector<glm::vec2> path;
    path.push_back(start);
    
    glm::vec2 apex = start, left = start, right = start;
    int apexIndex = 0, leftIndex = 0, rightIndex = 0;
    
    for (size_t i = 1; i < portals.size(); i++) 
    {
        const auto& portal = portals[i];
        
        // Right leg
        {
            float cross = (right - apex).x * (portal.second - apex).y - 
                          (right - apex).y * (portal.second - apex).x;
            
            if (cross < 0) // New is more right
            { 
                right = portal.second;
                rightIndex = i;
                
                // Check if right crosses over left
                cross = (apex - right).x * (left - right).y - 
                       (apex - right).y * (left - right).x;
                
                if (cross < 0) // Advance apex to left
                { 
                    path.push_back(left);
                    apex = left; apexIndex = leftIndex;
                    left = right = apex;
                    leftIndex = rightIndex = apexIndex;
                    i = apexIndex;
                    continue;
                }
            }
        }
        
        // Left leg
        {
            float cross = (left - apex).x * (portal.first - apex).y - 
                         (left - apex).y * (portal.first - apex).x;
            
            if (cross > 0) // New is more left
            { 
                left = portal.first;
                leftIndex = i;
                
                // Check if left crosses over right
                cross = (apex - left).x * (right - left).y - 
                       (apex - left).y * (right - left).x;
                
                if (cross < 0) // Advance apex to right
                { 
                    path.push_back(right);
                    apex = right; apexIndex = rightIndex;
                    left = right = apex;
                    leftIndex = rightIndex = apexIndex;
                    i = apexIndex;
                    continue;
                }
            }
        }
    }
    
    if (path.back() != end)
        path.push_back(end);
        
    return path;
}

void NavMeshComponent::UpdateDynamicObstacles(const std::vector<wolf::GameObject*>& obstacles)
{
    m_obstacles = obstacles;
    
    // Reset previous obstacles
    for (int polyId : m_obstacleAffectedPolygons)
    {
        if (polyId >= 0 && polyId < m_polygons.size())
        {
            m_polygons[polyId].walkable = true;
            auto it = m_originalVisibility.find(polyId);
            if (it != m_originalVisibility.end())
                m_polygons[polyId].visible = it->second;
        }
    }
    m_obstacleAffectedPolygons.clear();
    m_originalVisibility.clear();
    
    if (obstacles.empty())
        return;
    
    std::unordered_set<int> newlyAffected;
    
    // Get player
    wolf::GameObject* player = m_pPathfindingManager && m_pPathfindingManager->GetLabyrinthManager() ?
                               m_pPathfindingManager->GetLabyrinthManager()->GetPlayer() : nullptr;
    
    const float radius = 24.0f;
    const float radiusSq = radius * radius;
    
    // Process obstacles
    for (auto* obj : obstacles) 
    {
        if (!obj) continue;
        
        wolf::Transform2D* transform = obj->GetComponent<wolf::Transform2D>();
        if (!transform) continue;
        
        glm::vec2 pos = transform->GetGlobalPosition();
        bool isPlayer = (obj == player);
        
        // Check nearby cells
        int cellX = static_cast<int>(pos.x / SPATIAL_CELL_SIZE);
        int cellY = static_cast<int>(pos.y / SPATIAL_CELL_SIZE);
        
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                auto it = m_spatialHash.find({cellX + dx, cellY + dy});
                if (it == m_spatialHash.end()) continue;
                
                for (int polyId : it->second) 
                {
                    if (newlyAffected.count(polyId) > 0)
                        continue;
                        
                    const auto& poly = m_polygons[polyId];
                    
                    // Quick checks
                    if (IsPointInPolygon(pos, poly) || glm::distance2(poly.center, pos) <= radiusSq) 
                    {
                        ProcessAffectedPolygon(polyId, isPlayer, newlyAffected);
                        continue;
                    }
                    
                    // Check vertices
                    for (const auto& v : poly.vertices) 
                    {
                        if (glm::distance2(v, pos) <= radiusSq) 
                        {
                            ProcessAffectedPolygon(polyId, isPlayer, newlyAffected);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    m_obstacleAffectedPolygons.assign(newlyAffected.begin(), newlyAffected.end());
}

void NavMeshComponent::ProcessAffectedPolygon(int polyId, bool isPlayer, std::unordered_set<int>& affected)
{
    if (isPlayer)
    {
        if (m_originalVisibility.find(polyId) == m_originalVisibility.end())
            m_originalVisibility[polyId] = m_polygons[polyId].visible;
        m_polygons[polyId].visible = false;
    }
    else
    {
        m_polygons[polyId].walkable = false;
    }
    affected.insert(polyId);
}

void NavMeshComponent::DebugDraw() const 
{
    if (!m_debugDrawEnabled) return;
    
    GLShapesRenderer* renderer = GLShapesRenderer::GetInstance();
    if (!renderer) return;
    
    // Draw polygons
    glm::vec4 fillColor(0.0f, 0.3f, 0.0f, 0.2f);
    glm::vec4 edgeColor(0.0f, 1.0f, 0.0f, 0.6f);
    glm::vec4 boundaryColor(0.9f, 0.2f, 0.2f, 0.7f);
    
    for (const auto& poly : m_polygons) 
    {
        if (!poly.visible || !poly.walkable || poly.vertices.size() < 3)
            continue;
            
        // Draw fill
        const glm::vec2& v0 = poly.vertices[0];
        ColouredVertex2D first = {v0.x, v0.y, fillColor.r, fillColor.g, fillColor.b, fillColor.a};
        
        for (size_t i = 1; i < poly.vertices.size() - 1; i++) 
        {
            renderer->AddTriangle(
                first,
                {poly.vertices[i].x, poly.vertices[i].y, fillColor.r, fillColor.g, fillColor.b, fillColor.a},
                {poly.vertices[i+1].x, poly.vertices[i+1].y, fillColor.r, fillColor.g, fillColor.b, fillColor.a}
            );
        }
    }
    renderer->RenderAndDeleteTriangles();
    
    // Draw edges
    for (const auto& edge : m_edges) 
    {
        auto color = (edge.poly2 == -1) ? boundaryColor : edgeColor;
        renderer->AddLine(
            {edge.start.x, edge.start.y, color.r, color.g, color.b, color.a},
            {edge.end.x, edge.end.y, color.r, color.g, color.b, color.a}
        );
    }
    renderer->RenderAndDeleteLines();
}

bool NavMeshComponent::IsPathValid(const std::vector<glm::vec2>& path) const
{
    if (path.size() < 2) return false;
    
    for (size_t i = 0; i < path.size() - 1; i++) 
    {
        if (!LineOfSight(path[i], path[i+1]))
            return false;
            
        if (i > 0) 
        {
            for (const auto& edge : m_edges)
            {
                if (edge.poly2 == -1 && DistancePointToSegment(path[i], edge.start, edge.end) < 10.0f)
                    return false;
            }
        }
    }
    
    return true;
}

std::vector<glm::vec2> NavMeshComponent::CreateGridBasedPath(const glm::vec2& start, const glm::vec2& goal)
{
    if (!m_pPathfindingManager) return {};
    
    const float tileSize = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
    
    glm::ivec2 startTile = glm::ivec2(start) / static_cast<int>(tileSize);
    glm::ivec2 goalTile = glm::ivec2(goal) / static_cast<int>(tileSize);
    
    std::vector<glm::ivec2> gridPath = m_pPathfindingManager->FindPath(startTile, goalTile);
    std::vector<glm::vec2> worldPath;
    
    for (const auto& tile : gridPath)
        worldPath.push_back(glm::vec2(tile) * tileSize + glm::vec2(tileSize/2.0f));
    
    return worldPath;
}