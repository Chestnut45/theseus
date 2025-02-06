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
    m_vv2CollidingPoints.clear();
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
}

void LightComponent::Update(float p_fDelta) {
    // Update the origin point of the light's radius
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // std::vector<glm::vec2> vv2ColliderCorners = m_pCollider->GetWorldSpaceCorners();
    // for (int k = 0; k < vv2ColliderCorners.size(); k += 4) {
    //     glm::vec2 v2TopLeft = vv2ColliderCorners.at(k);
    //     glm::vec2 v2BotLeft = vv2ColliderCorners.at(k+1);
    //     glm::vec2 v2BotRight = vv2ColliderCorners.at(k+2);
    //     glm::vec2 v2TopRight = vv2ColliderCorners.at(k+3);

    //     // Draw the lines
    //     GLShapesRenderer::GetInstance()->AddLine({v2TopLeft.x, v2TopLeft.y}, {v2TopRight.x, v2TopRight.y}); // Top
    //     GLShapesRenderer::GetInstance()->AddLine({v2TopLeft.x, v2TopLeft.y}, {v2BotLeft.x, v2BotLeft.y}); // Left
    //     GLShapesRenderer::GetInstance()->AddLine({v2BotLeft.x, v2BotLeft.y}, {v2BotRight.x, v2BotRight.y}); // Bottom
    //     GLShapesRenderer::GetInstance()->AddLine({v2TopRight.x, v2TopRight.y}, {v2BotRight.x, v2BotRight.y}); // Right
    // }

    // Empty out the map of last frame's ray end points
    m_vv2CollidingPoints.clear();

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

                // And store those in a new rectangle
                vpRectanglesInAOE.push_back(wolf::Rectangle(v2TopLeft.x, v2TopLeft.y, v2BotRight.x, v2BotRight.y));
            }
        }
    }

    // !-- Note that the sweep lines go beyond the light's AOE so that they can reach the corners --!

    // Do a "sweep" around the light and check what corner points collide with it
    float fSweepLineLength = std::max(m_v2Radius.x, m_v2Radius.y);
    glm::vec2 v2BaseSweepLineEnd = {m_v2Origin.x, m_v2Origin.y + fSweepLineLength};
    glm::vec2 v2CurSweepLineEnd = {m_v2Origin.x, m_v2Origin.y + fSweepLineLength};

    // Go through all of the rectangles in the AOE
    for (wolf::Rectangle rect : vpRectanglesInAOE) {
        // Get the corners and use them to form the sides of the rectangle
        // !-- GetCorners() returns: top-left, top-right, bottom-left, bottom-right --!
        std::array<glm::vec2, 4> arv2Corners = rect.GetCorners();

        glm::vec2 v2TopStart = arv2Corners[0]; // Top
        glm::vec2 v2TopEnd = arv2Corners[1];

        glm::vec2 v2BotStart = arv2Corners[2]; // Bottom
        glm::vec2 v2BotEnd = arv2Corners[3];

        glm::vec2 v2LeftStart = arv2Corners[0]; // Left
        glm::vec2 v2LeftEnd = arv2Corners[2];

        glm::vec2 v2RightStart = arv2Corners[1]; // Right
        glm::vec2 v2RightEnd = arv2Corners[3];

        // Do a collision test between each of the corners and the four sides of the rectangle
        for (glm::vec2 v2Corner : arv2Corners) {
            // !-- Because a light can't collide with a given side and the side directly across
            //     from it at the same time, (if we collide with the left we CANNOT collide with
            //     the right and if we collide with the top we CANNOT collide with the bottom)
            //     we group these sides together and ensure that if a collision happens with one
            //     the other is NOT checked. (This keeps the light from going through things) --!

            // !-- Need a way to prefer left over right and vise versa depending on the distance to the light --!
            // So if left is closer, check it first and take it if it collides, if right is closer check it first
            // Do the same for top and bottom

            // Left and right
            std::pair<bool, glm::vec2> bv2LeftResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2LeftStart, v2LeftEnd);
            std::pair<bool, glm::vec2> bv2RightResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2RightStart, v2RightEnd);
            if (bv2LeftResult.first) {  // Check left
                m_vv2CollidingPoints.push_back(bv2LeftResult.second);
            }
            else if (bv2RightResult.first) { // Check right
                m_vv2CollidingPoints.push_back(bv2RightResult.second);
            }

            // Top and bottom
            std::pair<bool, glm::vec2> bv2TopResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2TopStart, v2TopEnd);
            std::pair<bool, glm::vec2> bv2BotResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2BotStart, v2BotEnd);
            if (bv2TopResult.first) { // Check top
                m_vv2CollidingPoints.push_back(bv2TopResult.second);
            }
            else if (bv2BotResult.first) { // Check bottom
                m_vv2CollidingPoints.push_back(bv2BotResult.second);
            }
        }
    }

    // !-- Need to sort the colliding points by angle --!

    // Form triangles using the two points that form each side and the origin
    while (!m_vv2CollidingPoints.empty() && static_cast<int>(m_vv2CollidingPoints.size() % 2 == 0)) {
        glm::vec2 v2Point1 = m_vv2CollidingPoints.back();
        m_vv2CollidingPoints.pop_back();

        glm::vec2 v2Point2 = m_vv2CollidingPoints.back();
        m_vv2CollidingPoints.pop_back();

        GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2Point1.x, v2Point1.y}, {v2Point2.x, v2Point2.y});
    }

    //Form a final triangle from the first and last points in m_iv2CollidingPoints and the origin
    // glm::vec2 v2FirstPoint = m_iv2CollidingPoints.at(0);
    // glm::vec2 v2LastPoint = m_iv2CollidingPoints.at(m_iEndOfCollidingPointsMap - 1);

    // GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2FirstPoint.x, v2FirstPoint.y}, {v2LastPoint.x, v2LastPoint.y});

}

bool LightComponent::SweepLinePointCollisionTest(const glm::vec2& p_v2LineEnd, const glm::vec2& p_v2Point) {
    // If the point is on the line between the origin and the end of the sweep line
    if (p_v2Point.x <= std::max(m_v2Origin.x, p_v2LineEnd.x) && p_v2Point.x >= std::min(m_v2Origin.x, p_v2LineEnd.x) &&
        p_v2Point.y <= std::max(m_v2Origin.y, p_v2LineEnd.y) && p_v2Point.y >= std::min(m_v2Origin.y, p_v2LineEnd.y))
    {
        return true; // They're colliding
    }

    // Otherwise, they're not
    return false;
}

std::pair<bool, glm::vec2> LightComponent::LineToCornerRectSideCollisionTest(const glm::vec2& p_v2CornerPoint, const glm::vec2& p_v2SideStart, const glm::vec2& p_v2SideEnd) {
    // Draw a line from the origin to the corner point parameter
    // Draw a line from side start to side end
    // If the two intersect, return the point of intersection
    // Save the point of intersection and use it to draw a light line later

    // Variables that will be overriden if the lines intersect
    bool bIntersected = false;
    glm::vec2 v2Intersect = {0.0f, 0.0f};

    // Calculate distance to intersection point
    float fDistX = ((p_v2SideEnd.x - p_v2SideStart.x) * (m_v2Origin.y - p_v2SideStart.y) - (p_v2SideEnd.y - p_v2SideStart.y) * (m_v2Origin.x - p_v2SideStart.x)) 
                / ((p_v2SideEnd.y - p_v2SideStart.y) * (p_v2CornerPoint.x - m_v2Origin.x) - (p_v2SideEnd.x - p_v2SideStart.x) * (p_v2CornerPoint.y - m_v2Origin.y));

    float fDistY = ((p_v2CornerPoint.x - m_v2Origin.x) * (m_v2Origin.y - p_v2SideStart.y) - (p_v2CornerPoint.y - m_v2Origin.y) * (m_v2Origin.x - p_v2SideStart.x))
                / ((p_v2SideEnd.y - p_v2SideStart.y) * (p_v2CornerPoint.x - m_v2Origin.x) - (p_v2SideEnd.x - p_v2SideEnd.x) * (p_v2CornerPoint.y - m_v2Origin.y));

    // Check for collision
    if (fDistX >= 0 && fDistX <= 1 && fDistY >= 0 && fDistY <= 1) {
        bIntersected = true;
        v2Intersect.x = m_v2Origin.x + (fDistX * (p_v2CornerPoint.x - m_v2Origin.x));
        v2Intersect.y = m_v2Origin.y + (fDistX * (p_v2CornerPoint.y - m_v2Origin.y));
    }

    // Return the results
    return {bIntersected, v2Intersect};
}