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
    m_vfv2CollidingPoints.clear();
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
    m_vfv2CollidingPoints.clear();

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

        /*----------------------------------------------------------------------------------//
        //    Because a light can't collide with a given side and the side directly across
        //    from it at the same time, (if we collide with the left we CANNOT collide with
        //    the right and if we collide with the top we CANNOT collide with the bottom)
        //    we group these sides together and ensure that if a collision happens with one
        //    the other is NOT checked. This will prevent the light from going through an
        //    object as long as we check the side NEAREST to the light FIRST.
        //---------------------------------------------------------------------------------*/
       
        // Figure out the approximate location of the object in relation to the light source
        // so that we know which sides of the box should be prioritized
        glm::vec2 v2RectPos = glm::vec4((v2TopStart.x + v2TopEnd.x) * 0.5f, (v2TopStart.y + v2BotStart.y) * 0.5f, 0, 1) * m_pTransform->GetGlobalMatrix();
        glm::vec2 v2LightPos = m_pTransform->GetGlobalPosition();

        // Bool variables to keep track of which sides should be checked first
        bool bFavourLeft;
        bool bFavourTop;

        // If the light is to the left of the rectangle, favour the left
        if (v2LightPos.x < v2RectPos.x) {
            bFavourLeft = true;
        }
        else {
            // Otherwise, favour the right
            bFavourLeft = false;
        }

        // If the light is above the rectangle, favour the top
        if (v2LightPos.y > v2RectPos.y) {
            bFavourTop = true;
        }
        else {
            // Otherwise, favour the bottom
            bFavourTop = false;
        }

        // Do a collision test between each of the corners and the four sides of the rectangle
        for (glm::vec2 v2Corner : arv2Corners) {
            // We're going to want to sort the collision points by the angle of the line between
            // them and the origin of the light later, so we create a variable to hold that value
            // whenever we recalculate it.
            float fAngleOfIntersect = 0.0f;

            // Perform the collision test for the left and right
            std::pair<bool, glm::vec2> bv2LeftResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2LeftStart, v2LeftEnd);
            std::pair<bool, glm::vec2> bv2RightResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2RightStart, v2RightEnd);

            // Check the favoured side first
            if (bFavourLeft) {
                // Prefer left
                if (bv2LeftResult.first) {  // Check left
                    // Calculate the cosine angle of intersection
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2LeftResult.second);

                    // Then add the point to the collision points vector
                    m_vfv2CollidingPoints.push_back({bv2LeftResult.second, fAngleOfIntersect});
                }
                else if (bv2RightResult.first) { // Check right
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2RightResult.second);
                    m_vfv2CollidingPoints.push_back({bv2RightResult.second, fAngleOfIntersect});
                }
            }
            else {
                // Prefer right
                if (bv2RightResult.first) {  // Check right
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2RightResult.second);
                    m_vfv2CollidingPoints.push_back({bv2RightResult.second, fAngleOfIntersect});
                }
                else if (bv2LeftResult.first) { // Check left
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2LeftResult.second);
                    m_vfv2CollidingPoints.push_back({bv2LeftResult.second, fAngleOfIntersect});
                }
            }

            // Do the same for the top and bottom
            std::pair<bool, glm::vec2> bv2TopResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2TopStart, v2TopEnd);
            std::pair<bool, glm::vec2> bv2BotResult = this->LineToCornerRectSideCollisionTest(v2Corner, v2BotStart, v2BotEnd);

            if (bFavourTop) {
                // Prefer top
                if (bv2TopResult.first) { // Check top
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2TopResult.second);
                    m_vfv2CollidingPoints.push_back({bv2TopResult.second, fAngleOfIntersect});
                }
                else if (bv2BotResult.first) { // Check bottom
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2BotResult.second);
                    m_vfv2CollidingPoints.push_back({bv2BotResult.second, fAngleOfIntersect});
                }
            }
            else {
                // Prefer bottom
                if (bv2BotResult.first) { // Check bottom
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2BotResult.second);
                    m_vfv2CollidingPoints.push_back({bv2BotResult.second, fAngleOfIntersect});
                }
                else if (bv2TopResult.first) { // Check top
                    fAngleOfIntersect = CalculateCosAngleOfIntersection(bv2TopResult.second);
                    m_vfv2CollidingPoints.push_back({bv2TopResult.second, fAngleOfIntersect});
                }
            }
        }
    }

    // Sort the collision points by the slopes of their intersection lines
    std::sort(m_vfv2CollidingPoints.begin(), m_vfv2CollidingPoints.end(), LightComponent::CompareVec2FloatPair);

    // Get the first collison point (we'll need it for the final triangle)
    glm::vec2 v2FirstPoint = m_vfv2CollidingPoints.back().first;

    // Form triangles using the two points that form each side and the origin
    while (m_vfv2CollidingPoints.size() != 1) {
        glm::vec2 v2Point1 = m_vfv2CollidingPoints.back().first;
        m_vfv2CollidingPoints.pop_back();

        glm::vec2 v2Point2 = m_vfv2CollidingPoints.back().first;
        // We don't pop the second point because we want the triangles to connect to each other

        GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2Point1.x, v2Point1.y}, {v2Point2.x, v2Point2.y});
    }

    // Pop the last point and use it to form the final triangle
    glm::vec2 v2LastPoint = m_vfv2CollidingPoints.back().first;
    m_vfv2CollidingPoints.pop_back();

    // Form a final triangle from the first and last points in m_iv2CollidingPoints and the origin
    GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2FirstPoint.x, v2FirstPoint.y}, {v2LastPoint.x, v2LastPoint.y});

}

// Check if the line between the light and a given corner point intersects with a given side of a rectangle
std::pair<bool, glm::vec2> LightComponent::LineToCornerRectSideCollisionTest(const glm::vec2& p_v2CornerPoint, const glm::vec2& p_v2SideStart, const glm::vec2& p_v2SideEnd) {
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

float LightComponent::CalculateCosAngleOfIntersection(const glm::vec2& p_v2Intersect) {
    // Calculate the slope of the line and use that as the hypotenuse
    glm::vec2 v2Direction = p_v2Intersect - m_v2Origin;

    // Calculate the cosine angle
    float fAngle = glm::atan(v2Direction.x, v2Direction.y);

    // Return the angle
    return fAngle;
}

// Compares two glm::vec2-float pairs and returns the one with the largest float value (break ties using the y coordinate)
bool LightComponent::CompareVec2FloatPair(std::pair<glm::vec2, float> p_v2fA, std::pair<glm::vec2, float> p_v2fB) {
    if (p_v2fA.second > p_v2fB.second) {
        return true;
    }

    return false;
}