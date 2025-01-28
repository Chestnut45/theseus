#include "LightComponent.h"
#include <vector>
#include <map>

int LightComponent::m_iNextIDNum = 0;

LightComponent::LightComponent(const glm::vec4& p_v4Color, const glm::vec2& p_v2Radius, bool p_bCanMove) 
    : m_v4Color(p_v4Color), m_v2Radius(p_v2Radius), m_iIDNum(m_iNextIDNum), m_bCanMove(p_bCanMove)
{

    // Retrieve the transform
    m_pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
    m_v2Origin = m_pTransform->GetGlobalPosition();

    // Retrieve the scene
    m_pScene = &this->GetGameObject()->GetScene();

    // Get the next ID number ready
    m_iNextIDNum++;
}

LightComponent::~LightComponent() {

}

void LightComponent::Update(float p_fDelta) {
    // Update the origin point of the light's radius
    m_v2Origin = m_pTransform->GetGlobalPosition();

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
    float fSweepLineLength = std::max(m_v2Radius.x, m_v2Radius.y);
    glm::vec2 v2SweepLineEnd = {m_v2Origin.x, m_v2Origin.y + fSweepLineLength};

    // Index var to make sure that all of the points in the colliding points map are unique
    m_iEndOfCollidingPointsMap = 0;

    // Sweep around the origin point
    for (int i = 0; i < 360; i++) {
        // Compute the new end point of the line
        v2SweepLineEnd.x = glm::cos(-i) * (v2SweepLineEnd.x - m_v2Origin.x) - glm::sin(-i) * (v2SweepLineEnd.y - m_v2Origin.y) + m_v2Origin.x;
        v2SweepLineEnd.y = glm::sin(-i) * (v2SweepLineEnd.x - m_v2Origin.x) + glm::cos(-i) * (v2SweepLineEnd.y - m_v2Origin.y) + m_v2Origin.y;

        // Go through all of the colliders in the AOE
        for (wolf::Rectangle rect : m_vpCollidersInAOE) {
            // Go through all of the points in said collider
            for (glm::vec2 point : rect.GetCorners()) {
                // If a point intersects with the sweep line
                if (SweepLinePointCollisionTest(v2SweepLineEnd, point)) {
                    // Add it to the collision points map
                    m_iv2CollidingPoints.insert({m_iEndOfCollidingPointsMap, point});
                    m_iEndOfCollidingPointsMap++;
                }
            }
        }
    }  

    // Form triangles using the two points that form each side and the origin

    /*
    var endpoints;   # list of endpoints, sorted by angle
    var open = [];   # list of walls the sweep line intersects

    loop over endpoints:
        remember which wall is nearest
        add any walls that BEGIN at this endpoint to 'walls'
        remove any walls that END at this endpoint from 'walls'
        
        figure out which wall is now nearest
        if the nearest wall changed:
            fill the current triangle and begin a new one
    */
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