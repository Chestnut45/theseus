#include "LightComponent.h"
#include <vector>

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

    // Empty out the vector of colliders in the light's AOE
    m_vpCollidersInAOE.empty();

    // Find the colliders that are inside the area of effect by iterating through the colliders in the scene
    for (auto&& [_, collider] : m_pScene->Each<ColliderComponent>()) {
        // If the collider is active
        if (collider.IsActive()) {
            // Get all of it's collider boxes
            std::vector<wolf::Rectangle> vColliderBoxes = collider.GetColliderBoxes();
            
            // Iterate through and figure out which ones are in the area of effect
            for (wolf::Rectangle rect : vColliderBoxes) {
                if (AABBCollisionTest(rect)) {
                    m_vpCollidersInAOE.push_back(&rect);
                }
            }
        }
    }

    // Do a "sweep" around the light and check what corner points collide with it
    float fSweepLineLength = std::max(m_v2Radius.x, m_v2Radius.y);

    std::vector<glm::vec2> vv2CollidingPoints;
    

    // Figure out which "sides" of the object are closest to the light

    // Form triangles using the two points that form each side and the origin

    // Extend the rays to go beyond the points so that we can hit the back walls

}