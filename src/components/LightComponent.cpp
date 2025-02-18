#include "LightComponent.h"
#include <vector>
#include <map>
#include <GLShapesRenderer.h>
#include <ColliderManager.h>

int LightComponent::m_iNextIDNum = 0;

LightComponent::LightComponent(const glm::vec4& p_v4Color, const glm::vec2& p_v2Radius, bool p_bCanMove) 
    : m_v4Color(p_v4Color), m_v2Radius(p_v2Radius), m_iIDNum(m_iNextIDNum), m_bCanMove(p_bCanMove)
{
    // Get the next ID number ready
    m_iNextIDNum++;
}

LightComponent::~LightComponent() {
    // Clear out the colliding points map
    m_vv2fCollidingPoints.clear();
}

void LightComponent::Init() {
    // Retrieve the transform
    m_pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Retrieve the scene
    m_pScene = &this->GetGameObject()->GetScene();

    // Add the light collider
    m_pCollider = &this->GetGameObject()->AddComponent<ColliderComponent>(ColliderComponent::NONE, false, true);
    m_pCollider->AddColliderBox(m_v2Radius, glm::vec2(-m_v2Radius.x / 2.0f, m_v2Radius.y / 2.0f));

    // Retrieve the labyrinth manager
    for (auto&& [_, lbmg] : this->m_pScene->Each<LabyrinthManager>())
    {
        m_pLabyrinthManager = &lbmg;
        break;
    }
}

void LightComponent::Update(float p_fDelta) {
    // Update the origin point of the light's radius
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Empty out the map of last frame's ray end points
    m_vv2fCollidingPoints.clear();

    // Create a vector to hold all of the colliders that are in the light's AOE
    std::vector<wolf::Rectangle> vpRectanglesInAOE;

    // Find the colliders that are inside the area of effect by iterating through the colliders in the scene
    for (auto&& [_, collider] : m_pScene->Each<ColliderComponent>()) {
        // If the collider is active and in this area of effect
        if (collider.IsActive() && ColliderManager::StaticMethodIsColliding(*m_pCollider, collider, p_fDelta)) {
            
            // Go through the corner points of each rectangle in the collider
            std::vector<glm::vec2> vv2ColliderCorners = collider.GetWorldSpaceCorners();
            for (int k = 0; k < vv2ColliderCorners.size(); k += 4) {
                // Find the top left and bottom right points
                glm::vec2 v2TopLeft = vv2ColliderCorners.at(k);
                glm::vec2 v2BotRight = vv2ColliderCorners.at(k + 2);

                // Check if the light's origin is inside of the rectangle
                if (m_v2Origin.x > v2TopLeft.x && m_v2Origin.x < v2BotRight.x && m_v2Origin.y > v2BotRight.y && m_v2Origin.y < v2TopLeft.y) {
                    // If it is, we do not want to cast rays to its corners unless it is the light's AOE collider
                    if (this->GetGameObject()->GetID() != collider.GetGameObject()->GetID()) {
                        continue;
                    }
                }

                // Check if the rectangle is completely outside of the light's radius
                // (This can happen because wall colliders are grouped by chunk)
                if ((v2TopLeft.x > m_v2Origin.x + m_v2Radius.x * 0.5f) ||
                    (v2TopLeft.y < m_v2Origin.y - m_v2Radius.y * 0.5f) ||
                    (v2BotRight.x < m_v2Origin.x - m_v2Radius.x * 0.5f) ||
                    (v2BotRight.y > m_v2Origin.y + m_v2Radius.y * 0.5f))
                {
                    // If it is, we do not want to cast rays to it
                    continue;
                }

                // If we've made it this far, we send a copy of this rectangle to the next phase of the collision testing
                vpRectanglesInAOE.push_back(wolf::Rectangle(v2TopLeft.x, v2TopLeft.y, v2BotRight.x, v2BotRight.y));
            }
        }
    }

    // Go through all of the rectangles in the AOE
    for (wolf::Rectangle rect : vpRectanglesInAOE) {
        // Get the corners of the rectangle
        // !-- GetCorners() returns: top-left, top-right, bottom-left, bottom-right --!
        std::array<glm::vec2, 4> arv2Corners = rect.GetCorners();

        // And store them in variables for easy readability
        glm::vec2 v2TopLeft = arv2Corners[0];
        glm::vec2 v2TopRight = arv2Corners[1];
        glm::vec2 v2BotLeft = arv2Corners[2];
        glm::vec2 v2BotRight = arv2Corners[3];
       
        // Figure out the approximate location of the rectangle in relation to the light source
        glm::vec2 v2RectPos = glm::vec4((v2TopLeft.x + v2TopRight.x) * 0.5f, (v2TopLeft.y + v2BotLeft.y) * 0.5f, 0, 1) * m_pTransform->GetGlobalMatrix();
        RoughPosition enRoughPos = this->CalculateRoughObjPosition(v2RectPos);

        // Use the said location to figure out which sides of the rectangle the light could
        // be hitting and which sides can be safely ignored
        switch (enRoughPos) {
            case TOP_LEFT:
                // Shoot a line to the bottom-left, bottom-right, and top-right corners
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

                /* --------------------------------------------------------------------------------------
                // The points that are furthest away from the light when the object is in one of the
                // diagonal positions (TOP_LEFT, TOP_RIGHT, BOT_LEFT, BOT_RIGHT) have the potential
                // to cause the light to shoot a ray through the object so we check those points against
                // the side directly across from them before they get added to the colliding points vector
                // --------------------------------------------------------------------------------------- */
                this->CheckForCollisionAndAdd(v2BotLeft, {v2TopRight, v2BotRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2BotLeft, v2BotRight});
        
            break;
            
            case TOP_CENTER:
                // Shoot a line to the bottom-left and bottom-right corners
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

            break;
            
            case TOP_RIGHT:
                // Shoot a line to the top-left, bottom-left, and bottom-right corners
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2BotRight, {v2TopLeft, v2BotLeft});
                this->CheckForCollisionAndAdd(v2TopLeft, {v2BotLeft, v2BotRight});

            break;
            
            case MID_LEFT:
                // Shoot a line to the top-right and bottom-right corners
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2BotLeft, v2BotRight});

            break;
            
            case MID_RIGHT:
                // Shoot a line to the top-left and bottom-left corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2BotLeft, {v2BotLeft, v2BotRight});

            break;
            
            case BOT_LEFT:
                // Shoot a line to the top-left, top-right, and bottom-right corners
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopRight, v2BotRight});
                this->CheckForCollisionAndAdd(v2BotRight, {v2TopLeft, v2TopRight});

            break;
            
            case BOT_CENTER:
                // Shoot a line to the top-left and top-right corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2TopRight});

            break;
            
            case BOT_RIGHT:
                // Shoot a line to the top-right, top-left, and bottom-left corners
                this->CheckForCollisionAndAdd(v2TopLeft, {v2BotLeft, v2BotRight});

                // Make sure that the ray doesn't go through the rectangle
                this->CheckForCollisionAndAdd(v2BotLeft, {v2TopLeft, v2TopRight});
                this->CheckForCollisionAndAdd(v2TopRight, {v2TopLeft, v2BotLeft});

            break;

            case SELF:
                // Shoot a line to the four corners of the light's radius rectangle
                // (If these lines intersect something they'll be removed in the next pass)
                m_vv2fCollidingPoints.push_back({v2TopLeft, CalculateCosAngleOfIntersection(v2TopLeft)});
                m_vv2fCollidingPoints.push_back({v2TopRight, CalculateCosAngleOfIntersection(v2TopRight)});
                m_vv2fCollidingPoints.push_back({v2BotLeft, CalculateCosAngleOfIntersection(v2BotLeft)});
                m_vv2fCollidingPoints.push_back({v2BotRight, CalculateCosAngleOfIntersection(v2BotRight)});
                
            break;
        }
    }

    // We don't want to draw light rays that go THROUGH the rectangles in the scene
    // so we are going to need to check if any of our lines intersect with other colliders
    std::vector<std::pair<glm::vec2, float>> vv2fPointsToRemove;

    // Go through all of the rectangles in the AOE again
    for (wolf::Rectangle rect : vpRectanglesInAOE) {
        // Clear the list of points to remove
        vv2fPointsToRemove.clear();

        // Get the corner points
        std::array<glm::vec2, 4> arv2RectCorners = rect.GetCorners();

        // Construct the sides again
        glm::vec2 v2TopStart = arv2RectCorners[0]; // Top
        glm::vec2 v2TopEnd = arv2RectCorners[1];

        glm::vec2 v2BotStart = arv2RectCorners[2]; // Bottom
        glm::vec2 v2BotEnd = arv2RectCorners[3];

        glm::vec2 v2LeftStart = arv2RectCorners[0]; // Left
        glm::vec2 v2LeftEnd = arv2RectCorners[2];

        glm::vec2 v2RightStart = arv2RectCorners[1]; // Right
        glm::vec2 v2RightEnd = arv2RectCorners[3];

        // Determine if this rectangle is part of a wall tile
        bool bRectIsWall = CheckForWallAtPos({(v2TopStart.x + v2BotEnd.x) * 0.5f, (v2TopStart.y + v2BotEnd.y) * 0.5f});

        // Then go through all of the corner points that we KNOW we'll be casting a light ray to
        for (std::pair<glm::vec2, float> v2fCorner : m_vv2fCollidingPoints) {
            // Skip corner points that belong to the rectangle we're currently looking at
            if (std::find(arv2RectCorners.begin(), arv2RectCorners.end(), v2fCorner.first) != arv2RectCorners.end()) {
                continue;
            }

            // Check if the line between the origin and the corner point we're casting to intersects with another collider.
            if (this->LineToCornerRectSideCollisionTest(v2fCorner.first, v2LeftStart, v2LeftEnd).first ||
                this->LineToCornerRectSideCollisionTest(v2fCorner.first, v2RightStart, v2RightEnd).first ||
                this->LineToCornerRectSideCollisionTest(v2fCorner.first, v2TopStart, v2TopEnd).first ||
                this->LineToCornerRectSideCollisionTest(v2fCorner.first, v2BotStart, v2BotEnd).first)
            {
                // If it does, and the rectangle we're currently comparing points against is a wall tile
                if (bRectIsWall)
                {
                    // Check if offsetting the corner point slightly in any direction would put it within the
                    // comparison rectangle's bounds.
                    if ((v2fCorner.first.x + 1.0f > v2LeftStart.x && v2fCorner.first.x + 1.0f < v2LeftEnd.x) ||
                        (v2fCorner.first.x - 1.0f > v2LeftStart.x && v2fCorner.first.x - 1.0f < v2LeftEnd.x) ||
                        (v2fCorner.first.y + 1.0f > v2BotStart.y && v2fCorner.first.y + 1.0f < v2TopStart.y) ||
                        (v2fCorner.first.y - 1.0f > v2BotStart.y && v2fCorner.first.y - 1.0f < v2TopStart.y))
                    {
                        // If it would, then this corner point is shared by two wall tiles and shouldn't be deleted
                        continue;
                    }
                }
                
                // Otherwise, we remove the point
                vv2fPointsToRemove.push_back(v2fCorner);
            }
        }

        // Remove the points this rectangle collided with
        for (std::pair<glm::vec2, float> v2fBadCorner : vv2fPointsToRemove) {
            auto it = std::find(m_vv2fCollidingPoints.begin(), m_vv2fCollidingPoints.end(), v2fBadCorner);
            if (it != m_vv2fCollidingPoints.end()) {
                m_vv2fCollidingPoints.erase(it);
            }
        }
    }

    // Sort the collision points by the slopes of their intersection lines
    std::sort(m_vv2fCollidingPoints.begin(), m_vv2fCollidingPoints.end(), LightComponent::CompareVec2FloatPair);

    // Get the first collison point (we'll need it for the final triangle)
    glm::vec2 v2FirstPoint = m_vv2fCollidingPoints.back().first;

    // Form triangles using the two points that form each side and the origin
    while (m_vv2fCollidingPoints.size() != 1) {
        glm::vec2 v2Point1 = m_vv2fCollidingPoints.back().first;
        m_vv2fCollidingPoints.pop_back();

        glm::vec2 v2Point2 = m_vv2fCollidingPoints.back().first;
        // We don't pop the second point because we want the triangles to connect to each other

        GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x, m_v2Origin.y}, {v2Point1.x, v2Point1.y});

        //GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2Point1.x, v2Point1.y}, {v2Point2.x, v2Point2.y});
    }

    // Pop the last point and use it to form the final triangle
    glm::vec2 v2LastPoint = m_vv2fCollidingPoints.back().first;
    m_vv2fCollidingPoints.pop_back();

    // Form a final triangle from the first and last points in m_iv2CollidingPoints and the origin
    GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x, m_v2Origin.y}, {v2LastPoint.x, v2LastPoint.y});
    //GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2FirstPoint.x, v2FirstPoint.y}, {v2LastPoint.x, v2LastPoint.y});

}

// !-- NEED TO DO THIS TEST FOR ALL POINTS, NOT JUST THE DIAGONALS --!

// Determines if a ray can be shot to the given corner without intersecting the given side and adds the corner to the vector
// of colliding points if it can. If the ray does intersect the given side, then the point of intersection is added to the
// colliding points vector, instead
void LightComponent::CheckForCollisionAndAdd(const glm::vec2& p_v2Corner, std::pair<const glm::vec2&, const glm::vec2&> p_v2v2Side) {
    // Check that the corner point is not outside of the radius
    if (!(p_v2Corner.x > m_v2Origin.x + m_v2Radius.x * 0.5f) &&
        !(p_v2Corner.x < m_v2Origin.x - m_v2Radius.x * 0.5f) &&
        !(p_v2Corner.y > m_v2Origin.y + m_v2Radius.y * 0.5f) &&
        !(p_v2Corner.y < m_v2Origin.y - m_v2Radius.y * 0.5f))
    {
        // Then check if the ray we're casting goes through the rectangle this corner is a part of
        glm::vec2 v2FinalPoint = this->LineToCornerRectSideCollisionTest(p_v2Corner, p_v2v2Side.first, p_v2v2Side.second).second;
        m_vv2fCollidingPoints.push_back({v2FinalPoint, CalculateCosAngleOfIntersection(v2FinalPoint)});
    }
}

// Figure out the rough position of the object in relation to the light's origin point by treating the world
// as a 3x3 grid with the light in the center position
LightComponent::RoughPosition LightComponent::CalculateRoughObjPosition(const glm::vec2& p_v2ObjCenterPos) {
    // Test the diagonals first
    if (p_v2ObjCenterPos.x < m_v2Origin.x && p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x && p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_RIGHT;
    if (p_v2ObjCenterPos.x < m_v2Origin.x && p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x && p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_RIGHT;

    // Then the cardinal directions
    if (p_v2ObjCenterPos.y < m_v2Origin.y) return LightComponent::BOT_CENTER;
    if (p_v2ObjCenterPos.y > m_v2Origin.y) return LightComponent::TOP_CENTER;
    if (p_v2ObjCenterPos.x < m_v2Origin.x) return LightComponent::MID_LEFT;
    if (p_v2ObjCenterPos.x > m_v2Origin.x) return LightComponent::MID_RIGHT;

    // If none of those succeed then the the light is comparing against itself
    return SELF;
}

// Check if the line between the light and a given corner point intersects with a given side of a rectangle,
// and return the point of intersection if it does, and the original point if it does not.
// Uses the line-line intersection test from this address:
// https://www.jeffreythompson.org/collision-detection/line-line.php
std::pair<bool, glm::vec2> LightComponent::LineToCornerRectSideCollisionTest(const glm::vec2& p_v2CornerPoint, const glm::vec2& p_v2SideStart, const glm::vec2& p_v2SideEnd) {
    // Variables that will be overriden if the lines intersect
    bool bIntersected = false;
    glm::vec2 v2Intersect = p_v2CornerPoint;

    // Calculate distance to intersection point
    float fDistX = ((p_v2SideEnd.x - p_v2SideStart.x) * (m_v2Origin.y - p_v2SideStart.y) - (p_v2SideEnd.y - p_v2SideStart.y) * (m_v2Origin.x - p_v2SideStart.x)) 
                / ((p_v2SideEnd.y - p_v2SideStart.y) * (p_v2CornerPoint.x - m_v2Origin.x) - (p_v2SideEnd.x - p_v2SideStart.x) * (p_v2CornerPoint.y - m_v2Origin.y));

    float fDistY = ((p_v2CornerPoint.x - m_v2Origin.x) * (m_v2Origin.y - p_v2SideStart.y) - (p_v2CornerPoint.y - m_v2Origin.y) * (m_v2Origin.x - p_v2SideStart.x))
                / ((p_v2SideEnd.y - p_v2SideStart.y) * (p_v2CornerPoint.x - m_v2Origin.x) - (p_v2SideEnd.x - p_v2SideStart.x) * (p_v2CornerPoint.y - m_v2Origin.y));

    // Check for collision
    if (fDistX >= 0 && fDistX <= 1 && fDistY >= 0 && fDistY <= 1) {
        bIntersected = true;
        v2Intersect.x = m_v2Origin.x + (fDistX * (p_v2CornerPoint.x - m_v2Origin.x));
        v2Intersect.y = m_v2Origin.y + (fDistX * (p_v2CornerPoint.y - m_v2Origin.y));
    }

    // Return the results
    return {bIntersected, v2Intersect};
}

float LightComponent::CalculateCosAngleOfIntersection(const glm::vec2& p_v2Intersect) {
    // Calculate the slope of the line and use that as the hypotenuse
    glm::vec2 v2Direction = p_v2Intersect - m_v2Origin;

    // Calculate the cosine angle
    float fAngle = glm::atan(v2Direction.x, v2Direction.y);

    // Return the angle
    return fAngle;
}

bool LightComponent::CheckForWallAtPos(const glm::vec2& p_v2Pos) {
    // Convert the given world-space position to a tile position
    glm::vec2 v2TilePos = m_pLabyrinthManager->GetTilePosition(p_v2Pos);

    // Get the ID of the tile at that position
    int iTileID = m_pLabyrinthManager->GetTile(v2TilePos.x, v2TilePos.y);

    // And return true if it is a wall tile
    return (iTileID >= Tile::WallBottomLeft && iTileID <= Tile::WallTop);
}

// Compares two glm::vec2-float pairs and returns the one with the largest float value (break ties using the y coordinate)
bool LightComponent::CompareVec2FloatPair(std::pair<glm::vec2, float> p_v2fA, std::pair<glm::vec2, float> p_v2fB) {
    if (p_v2fA.second > p_v2fB.second) {
        return true;
    }

    return false;
}