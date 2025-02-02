#include "LightComponent.h"
#include <vector>
#include <map>
#include <GLShapesRenderer.h>

int LightComponent::m_iNextIDNum = 0;

LightComponent::LightComponent(const glm::vec4& p_v4Color, const glm::vec2& p_v2Radius, bool p_bCanMove) 
    : m_v4Color(p_v4Color), m_v2Radius(p_v2Radius), m_iIDNum(m_iNextIDNum), m_bCanMove(p_bCanMove)
{
    // Get the next ID number ready
    m_iNextIDNum++;
}

LightComponent::~LightComponent() {
    // Delete the radius rectangle
    delete(m_pRadiusRectangle);

    // And clear out the colliding points map
    m_iv2CollidingPoints.clear();
}

void LightComponent::Init() {
    // Retrieve the transform
    m_pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Retrieve the scene
    m_pScene = &this->GetGameObject()->GetScene();

    // Set up the radius rectangle
    m_pRadiusRectangle = new wolf::Rectangle(m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y + m_v2Radius.y / 2.0f,
                                                m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y - m_v2Radius.y / 2.0f);
}

void LightComponent::Update(float p_fDelta) {
    // Update the origin point of the light's radius
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // FOR DEBUGGING: Show the area of effect lines
    //GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y}, {m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y});
    //GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x, m_v2Origin.y - m_v2Radius.y / 2.0f}, {m_v2Origin.x, m_v2Origin.y + m_v2Radius.y / 2.0f});
    //GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y - m_v2Radius.y / 2.0f}, {m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y + m_v2Radius.y / 2.0f});
    //GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x - m_v2Radius.x / 2.0f, m_v2Origin.y + m_v2Radius.y / 2.0f}, {m_v2Origin.x + m_v2Radius.x / 2.0f, m_v2Origin.y - m_v2Radius.y / 2.0f});

    // Empty out the map of last frame's ray end points
    m_iv2CollidingPoints.clear();

    // Create a vector to hold all of the colliders that are in the light's AOE
    std::vector<wolf::Rectangle> m_vpCollidersInAOE;

    // Find the colliders that are inside the area of effect by iterating through the colliders in the scene
    for (auto&& [_, collider] : m_pScene->Each<ColliderComponent>()) {
        // If the collider is active
        if (collider.IsActive()) {
            // Get all of it's collider boxes
            std::vector<wolf::Rectangle> vColliderBoxes = collider.GetColliderBoxes();
            
            // Iterate through and figure out which ones are in the area of effect
            for (wolf::Rectangle rect : vColliderBoxes) {
                if (m_pRadiusRectangle->Intersects(rect)) {
                    m_vpCollidersInAOE.push_back(rect);
                }
            }
        }
    }

    // Do a "sweep" around the light and check what corner points collide with it
    float fSweepLineLength = std::max(m_v2Radius.x / 2.0f, m_v2Radius.y / 2.0f);
    glm::vec2 v2SweepLineEnd = {0.0f, 0.0f};

    // Index var to make sure that all of the points in the colliding points map are unique
    m_iEndOfCollidingPointsMap = 0;

    // Sweep around the origin point
    for (int i = 0; i <= 360; i++) {
        // Compute the new end point of the line
        v2SweepLineEnd.x = glm::cos(glm::radians(-i * 1.0f)) * (fSweepLineLength - m_v2Origin.x) - glm::sin(glm::radians(-i * 1.0f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.x;
        v2SweepLineEnd.y = glm::sin(glm::radians(-i * 1.0f)) * (fSweepLineLength - m_v2Origin.x) + glm::cos(glm::radians(-i * 1.0f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.y;

        // If the main sweep line hits then we also want to shoot off two slightly offset
        // sweep lines to make sure that we hit the wall behind edge colliders
        glm::vec2 v2LeftOffsetSLEnd;
        v2LeftOffsetSLEnd.x = glm::cos(glm::radians((-i * 1.0f) - 0.00001f)) * (fSweepLineLength - m_v2Origin.x) - glm::sin(glm::radians((-i * 1.0f) - 0.00001f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.x;
        v2LeftOffsetSLEnd.y = glm::sin(glm::radians((-i * 1.0f) - 0.00001f)) * (fSweepLineLength - m_v2Origin.x) + glm::cos(glm::radians((-i * 1.0f) - 0.00001f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.y;

        glm::vec2 v2RightOffsetSLEnd;
        v2RightOffsetSLEnd.x = glm::cos(glm::radians((-i * 1.0f) + 0.00001f)) * (fSweepLineLength - m_v2Origin.x) - glm::sin(glm::radians((-i * 1.0f) + 0.00001f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.x;
        v2RightOffsetSLEnd.y = glm::sin(glm::radians((-i * 1.0f) + 0.00001f)) * (fSweepLineLength - m_v2Origin.x) + glm::cos(glm::radians((-i * 1.0f) + 0.00001f)) * (fSweepLineLength - m_v2Origin.y) + m_v2Origin.y;

        // FOR DEBUGGING: Draw the sweep lines
        GLShapesRenderer::GetInstance()->AddLine({m_v2Origin.x, m_v2Origin.y}, {v2SweepLineEnd.x, v2SweepLineEnd.y});

        // Go through all of the colliders in the AOE
        for (wolf::Rectangle rect : m_vpCollidersInAOE) {
            // Go through all of the corners in said collider
            for (glm::vec2 point : rect.GetCorners()) {
                // If a corner intersects with the sweep line
                if (SweepLinePointCollisionTest(v2SweepLineEnd, point)) {
                    // Check for collision points that are slightly to the left and right of it
                    std::pair<bool, glm::vec2> bv2LeftCollidingPoint = this->SweepLineRectCollisionTest(v2LeftOffsetSLEnd, rect);
                    std::pair<bool, glm::vec2> bv2RightCollidingPoint = this->SweepLineRectCollisionTest(v2RightOffsetSLEnd, rect);

                    // Then add the points that we found to the collision map (if they exist)

                    // Left collision point
                    if (bv2LeftCollidingPoint.first) {
                        m_iv2CollidingPoints.insert({m_iEndOfCollidingPointsMap, bv2LeftCollidingPoint.second});
                        m_iEndOfCollidingPointsMap++;
                    }

                    // Corner collision point
                    m_iv2CollidingPoints.insert({m_iEndOfCollidingPointsMap, point});
                    m_iEndOfCollidingPointsMap++;

                    // Right collison point
                    if (bv2RightCollidingPoint.first) {
                        m_iv2CollidingPoints.insert({m_iEndOfCollidingPointsMap, bv2RightCollidingPoint.second});
                        m_iEndOfCollidingPointsMap++;
                    }
                }
            }
        }
    }  

    // Form triangles using the two points that form each side and the origin
    for (int j = 0; j < m_iEndOfCollidingPointsMap - 1; j++) {
        // Get the points at j and j+1
        glm::vec2 v2Point1 = m_iv2CollidingPoints.at(j);
        glm::vec2 v2Point2 = m_iv2CollidingPoints.at(j+1);

        // Add a triangle to the render list
        GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2Point1.x, v2Point1.y}, {v2Point2.x, v2Point2.y});
    }

    // Form a final triangle from the first and last points in m_iv2CollidingPoints and the origin
    //glm::vec2 v2FirstPoint = m_iv2CollidingPoints.at(0);
    //glm::vec2 v2LastPoint = m_iv2CollidingPoints.at(m_iEndOfCollidingPointsMap);

    //GLShapesRenderer::GetInstance()->AddTriangle({m_v2Origin.x, m_v2Origin.y}, {v2FirstPoint.x, v2FirstPoint.y}, {v2LastPoint.x, v2LastPoint.y});
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

// Returns a std::pair, bool to indicate if the two intersect and a vec2 as the point of collision
std::pair<bool, glm::vec2> LightComponent::SweepLineRectCollisionTest(const glm::vec2& p_v2LineEnd, const wolf::Rectangle& p_pRect) {
    // First convert the rectangle into a series of lines that represent each of the four sides
    glm::vec2 v2TopBegin = {p_pRect.m_left, p_pRect.m_top};
    glm::vec2 v2TopEnd = {p_pRect.m_right, p_pRect.m_top};

    glm::vec2 v2BotBegin = {p_pRect.m_left, p_pRect.m_bottom};
    glm::vec2 v2BotEnd = {p_pRect.m_right, p_pRect.m_bottom};

    glm::vec2 v2LeftBegin = {p_pRect.m_left, p_pRect.m_top};
    glm::vec2 v2LeftEnd = {p_pRect.m_left, p_pRect.m_bottom};

    glm::vec2 v2RightBegin = {p_pRect.m_right, p_pRect.m_top};
    glm::vec2 v2RightEnd = {p_pRect.m_right, p_pRect.m_bottom};

    // Then test for a collision between each of the sides
    bool bLineAndRectCollided = false;

    // Top
    float fTopDistX = ((v2TopEnd.x - v2TopBegin.x) * (m_v2Origin.y - v2TopBegin.y) - (v2TopEnd.y - v2TopBegin.y) * (m_v2Origin.x - v2TopBegin.x)) /
                            ((v2TopEnd.y - v2TopBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2TopEnd.x - v2TopBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));
                            
    float fTopDistY = ((p_v2LineEnd.x - m_v2Origin.x) * (m_v2Origin.y - v2TopEnd.y) - (p_v2LineEnd.y - m_v2Origin.y) * (m_v2Origin.x - v2TopBegin.x)) /
                                ((v2TopEnd.y - v2TopBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2TopEnd.x - v2TopBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));

    glm::vec2 v2TopIntersectionPoint = {0.0f, 0.0f};
    if (fTopDistX >= 0 && fTopDistX <= 1 && fTopDistY >= 0 && fTopDistY <= 1) {
        // They're intersecting
        v2TopIntersectionPoint = {m_v2Origin.x + (fTopDistX * (p_v2LineEnd.x - m_v2Origin.x)), m_v2Origin.y + (fTopDistX * (p_v2LineEnd.y - m_v2Origin.y))};
        bLineAndRectCollided = true;
    }

    // Left
    float fLeftDistX = ((v2LeftEnd.x - v2LeftBegin.x) * (m_v2Origin.y - v2LeftBegin.y) - (v2LeftEnd.y - v2LeftBegin.y) * (m_v2Origin.x - v2LeftBegin.x)) /
                        ((v2LeftEnd.y - v2LeftBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2LeftEnd.x - v2LeftBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));
                            
    float fLeftDistY = ((p_v2LineEnd.x - m_v2Origin.x) * (m_v2Origin.y - v2LeftEnd.y) - (p_v2LineEnd.y - m_v2Origin.y) * (m_v2Origin.x - v2LeftBegin.x)) /
                        ((v2LeftEnd.y - v2LeftBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2LeftEnd.x - v2LeftBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));

    glm::vec2 v2LeftIntersectionPoint = {0.0f, 0.0f};
    if (fLeftDistX >= 0 && fLeftDistX <= 1 && fLeftDistY >= 0 && fLeftDistY <= 1) {
        // They're intersecting
        v2LeftIntersectionPoint = {m_v2Origin.x + (fLeftDistX * (p_v2LineEnd.x - m_v2Origin.x)), m_v2Origin.y + (fLeftDistX * (p_v2LineEnd.y - m_v2Origin.y))};
        bLineAndRectCollided = true;
    }

    // Right
    float fRightDistX = ((v2RightEnd.x - v2RightBegin.x) * (m_v2Origin.y - v2RightBegin.y) - (v2RightEnd.y - v2RightBegin.y) * (m_v2Origin.x - v2RightBegin.x)) /
                        ((v2RightEnd.y - v2RightBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2RightEnd.x - v2RightBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));
                            
    float fRightDistY = ((p_v2LineEnd.x - m_v2Origin.x) * (m_v2Origin.y - v2RightEnd.y) - (p_v2LineEnd.y - m_v2Origin.y) * (m_v2Origin.x - v2RightBegin.x)) /
                        ((v2RightEnd.y - v2RightBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2RightEnd.x - v2RightBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));

    glm::vec2 v2RightIntersectionPoint = {0.0f, 0.0f};
    if (fRightDistX >= 0 && fRightDistX <= 1 && fRightDistY >= 0 && fRightDistY <= 1) {
        // They're intersecting
        v2RightIntersectionPoint = {m_v2Origin.x + (fRightDistX * (p_v2LineEnd.x - m_v2Origin.x)), m_v2Origin.y + (fRightDistX * (p_v2LineEnd.y - m_v2Origin.y))};
        bLineAndRectCollided = true;
    }

    // Bottom
    float fBottomDistX = ((v2BotEnd.x - v2BotBegin.x) * (m_v2Origin.y - v2BotBegin.y) - (v2BotEnd.y - v2BotBegin.y) * (m_v2Origin.x - v2BotBegin.x)) /
                        ((v2BotEnd.y - v2BotBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2BotEnd.x - v2BotBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));
                            
    float fBottomDistY = ((p_v2LineEnd.x - m_v2Origin.x) * (m_v2Origin.y - v2BotEnd.y) - (p_v2LineEnd.y - m_v2Origin.y) * (m_v2Origin.x - v2BotBegin.x)) /
                        ((v2BotEnd.y - v2BotBegin.y) * (p_v2LineEnd.x - m_v2Origin.x) - (v2BotEnd.x - v2BotBegin.x) * (p_v2LineEnd.y - m_v2Origin.y));

    glm::vec2 v2BottomIntersectionPoint = {0.0f, 0.0f};
    if (fBottomDistX >= 0 && fBottomDistX <= 1 && fBottomDistY >= 0 && fBottomDistY <= 1) {
        // They're intersecting
        v2BottomIntersectionPoint = {m_v2Origin.x + (fBottomDistX * (p_v2LineEnd.x - m_v2Origin.x)), m_v2Origin.y + (fBottomDistX * (p_v2LineEnd.y - m_v2Origin.y))};
        bLineAndRectCollided = true;
    }

    // If the sweep line and rectangle collided
    if (bLineAndRectCollided) {
        // We only want to keep the collision point that is closest to the origin as that will find us the closest wall
        float fDistToTop = glm::distance(m_v2Origin, v2TopIntersectionPoint);
        float fDistToLeft = glm::distance(m_v2Origin, v2LeftIntersectionPoint);
        float fDistToRight = glm::distance(m_v2Origin, v2RightIntersectionPoint);
        float fDistToBot = glm::distance(m_v2Origin, v2BottomIntersectionPoint);

        // So we find the intersection point that is closest to the origin
        float fSmallestDist = std::min(std::min(std::min(fDistToTop, fDistToLeft), fDistToRight), fDistToBot);
        
        // And return it with the bool value true so that we know we passed the intersection test
        if (fSmallestDist == fDistToTop) {
            return {true, v2TopIntersectionPoint};
        }
        else if (fSmallestDist == fDistToLeft) {
            return {true, v2LeftIntersectionPoint};
        }
        else if (fSmallestDist == fDistToRight) {
            return {true, v2RightIntersectionPoint};
        }
        else if (fSmallestDist == fDistToBot) {
            return {true, v2BottomIntersectionPoint};
        }
    }

    // Otherwise, we return false and a (0.0f, 0.0f) point
    return {false, glm::vec2(0.0f, 0.0f)};
}