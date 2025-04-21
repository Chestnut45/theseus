//-----------------------------------------------------------------------------
// File: ColliderManager.cpp
// Original Author: Nguyễn Minh Nhật
// Modifications / Optimizations: D'Anyil Landry, Youssef Ashraf
// Manages collider collisions.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
//-----------------------------------------------------------------------------

#include "ColliderManager.h"

#include <MinitaurController.h>
#include <GorgonController.h>
#include <PlayerController.h>

#include <ThrowableObjectComponent.h>
#include <W_Audio.h>

ColliderManager::ColliderManager(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

ColliderManager::~ColliderManager()
{
    this->m_scene = nullptr;
}

void ColliderManager::Update(float p_delta)
{
    this->CheckCornerCollision(p_delta);
    this->CheckCollisions(p_delta);
    this->RemoveFlagged();
}

// Remove objects flagged for destruction
void ColliderManager::RemoveFlagged()
{
    for (int i = 0; i < this->m_vToBeDestroyed.size(); i++)
    {
        auto pObject = m_scene->GetObject(m_vToBeDestroyed.at(i));

        // TODO: SFX code should not be here, but it's release time! ;)

        // Detect when throwable objects are destroyed and fire off sfx
        if (pObject->HasAll<ThrowableObjectComponent>())
        {
            wolf::Audio::Play("data/sounds/sfx_throwable_break.wav", 0.32f);
        }

        // Detect when fireballs are destroyed and fire off sfx
        if (auto* pAnim = pObject->GetComponent<AnimatedSprite2D>())
        {
            if (pAnim->GetCurrentAnimation()->m_strName == "burn")
            {
                // Ensure falloff for potentially stacked sounds
                wolf::Audio::Play("data/sounds/sfx_fireball_extinguish.wav", 0.6f);
            }
        }

        this->m_scene->DeleteObject(this->m_vToBeDestroyed.at(i));
    }
    this->m_vToBeDestroyed.clear();
}

// Iterate through collider components to check for collisions
void ColliderManager::CheckCollisions(float p_delta)
{
    // Early exit if not enough colliders
    if (ColliderComponent::s_iComponentCount < 2) return;

    // Iterate through all collider components
    int i = 0;
    for (auto&& [id1, collider1] : this->m_scene->Each<ColliderComponent>())
    {
        i++;

        // Skip inactive or flagged colliders
        if (!collider1.IsActive()) continue;

        // Skip occluders
        if (collider1.IsOccluder()) continue;

        // Cache properties for collider1
        const bool isHitbox1 = collider1.IsHitbox();
        const bool isDestroyedOnCollision1 = collider1.IsDestroyedOnCollision();
        const bool isSolidEnemy1 = collider1.GetGameObject()->HasAny<MinitaurController, GorgonController>();
        const bool isPlayer1 = collider1.GetGameObject()->HasAny<PlayerController>();

        // Iterate through remaining colliders after i
        for (auto&& [id2, collider2] : this->m_scene->Each<ColliderComponent>() | std::views::drop(i))
        {

            // Skip inactive or flagged colliders
            if (!collider2.IsActive()) continue;

            // Skip occluders
            if (collider2.IsOccluder()) continue;

            // Cache properties for collider2
            const bool isHitbox2 = collider2.IsHitbox();
            const bool isDestroyedOnCollision2 = collider2.IsDestroyedOnCollision();
            const bool isSolidEnemy2 = collider2.GetGameObject()->HasAny<MinitaurController, GorgonController>();
            const bool isPlayer2 = collider2.GetGameObject()->HasAny<PlayerController>();

            // Collision check conditions
            bool isCollisionCheckRequired =
                (isHitbox1 && isHitbox2) ||
                (isDestroyedOnCollision1 && isHitbox2) ||
                (isDestroyedOnCollision2 && isHitbox1);

            if (!isCollisionCheckRequired) continue;

            // Skip collisions between enemies to avoid them sticking
            if (isSolidEnemy1 && isSolidEnemy2) continue;

            // Skip collisions between enemies and the player
            if ((isPlayer1 && isSolidEnemy2) || (isPlayer2 && isSolidEnemy1)) continue;

            // Perform collision check
            if (this->IsCollidingInternalUse(collider1, collider2, p_delta))
            {
                // Flag collider1 for destruction if needed
                if (isDestroyedOnCollision1 && !collider1.m_bIsFlaggedForDestruction)
                {
                    collider1.m_bIsFlaggedForDestruction = true;
                    this->m_vToBeDestroyed.push_back(id1);
                }

                // Flag collider2 for destruction if needed
                if (isDestroyedOnCollision2 && !collider2.m_bIsFlaggedForDestruction)
                {
                    collider2.m_bIsFlaggedForDestruction = true;
                    this->m_vToBeDestroyed.push_back(id2);
                }
            }
        }
    }
}


bool ColliderManager::IsColliding(ColliderComponent& p_colliderComponent1, ColliderComponent& p_colliderComponent2, float p_delta)
{
    // Grab object pointers
    auto* pObj1 = p_colliderComponent1.GetGameObject();
    auto* pObj2 = p_colliderComponent2.GetGameObject();

    // Skip ignored IDs
    if ((p_colliderComponent1.m_IgnoreID == pObj2->GetID()) ||
        (p_colliderComponent2.m_IgnoreID == pObj1->GetID()))
    {
        return false;
    }
    
    // Grab transform pointers
    auto* pTrans1 = pObj1->GetComponent<wolf::Transform2D>();
    auto* pTrans2 = pObj2->GetComponent<wolf::Transform2D>();

    // Grab velocity pointers
    auto* pVel1 = pObj1->GetComponent<VelocityComponent>();
    auto* pVel2 = pObj2->GetComponent<VelocityComponent>();
    
    glm::vec2 scale1 = p_colliderComponent1.IsRelative() ? pTrans1->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    glm::vec2 scale2 = p_colliderComponent2.IsRelative() ? pTrans2->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    
    glm::vec2 objTranslation1 = pTrans1->GetGlobalPosition();
    glm::vec2 objTranslation2 = pTrans2->GetGlobalPosition();
    
    for(const wolf::Rectangle& collider1 : p_colliderComponent1.GetColliderBoxes())
    { 
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight()) * scale1;
        glm::vec2 offset1 = collider1.GetPosition() * scale1;            
        glm::vec2 translation1 = objTranslation1 + offset1;

        for(const wolf::Rectangle& collider2 : p_colliderComponent2.GetColliderBoxes())
        {                
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight()) * scale2;
            glm::vec2 offset2 = collider2.GetPosition() * scale2;        
            glm::vec2 translation2 = objTranslation2 + offset2;

            if(this->CustomAABB(translation1, translation2, dimensions1, dimensions2, pVel1, pVel2, p_delta) > 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool ColliderManager::StaticMethodIsColliding(ColliderComponent& p_colliderComponent1, ColliderComponent& p_colliderComponent2, float p_delta)
{
    // Grab object pointers
    auto* pObj1 = p_colliderComponent1.GetGameObject();
    auto* pObj2 = p_colliderComponent2.GetGameObject();

    // Skip ignored IDs
    if ((p_colliderComponent1.m_IgnoreID == pObj2->GetID()) ||
        (p_colliderComponent2.m_IgnoreID == pObj1->GetID()))
    {
        return false;
    }
    
    // Grab transform pointers
    auto* pTrans1 = pObj1->GetComponent<wolf::Transform2D>();
    auto* pTrans2 = pObj2->GetComponent<wolf::Transform2D>();

    // Grab velocity pointers
    auto* pVel1 = pObj1->GetComponent<VelocityComponent>();
    auto* pVel2 = pObj2->GetComponent<VelocityComponent>();
    
    glm::vec2 scale1 = p_colliderComponent1.IsRelative() ? pTrans1->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    glm::vec2 scale2 = p_colliderComponent2.IsRelative() ? pTrans2->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    
    glm::vec2 objTranslation1 = pTrans1->GetGlobalPosition();
    glm::vec2 objTranslation2 = pTrans2->GetGlobalPosition();
    
    for(const wolf::Rectangle& collider1 : p_colliderComponent1.GetColliderBoxes())
    { 
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight()) * scale1;
        glm::vec2 offset1 = collider1.GetPosition() * scale1;            
        glm::vec2 translation1 = objTranslation1 + offset1;

        for(const wolf::Rectangle& collider2 : p_colliderComponent2.GetColliderBoxes())
        {                
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight()) * scale2;
            glm::vec2 offset2 = collider2.GetPosition() * scale2;        
            glm::vec2 translation2 = objTranslation2 + offset2;

            if(StaticMethodCustomAABB(translation1, translation2, dimensions1, dimensions2, pVel1, pVel2, p_delta) > 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool ColliderManager::StandardAABB(float left1, float right1, float top1, float bottom1, float left2, float right2, float top2, float bottom2)
{
    return !(
        right1  < left2     ||
        left1   > right2    ||
        bottom1 > top2      ||
        top1    < bottom2
    ); 
}

bool ColliderManager::CustomAABB(const glm::vec2& p_translation_1, const glm::vec2& p_translation_2, const glm::vec2& p_dimensions_1, const glm::vec2& p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    // Initial AABB check
    bool result = !(
        p_translation_1.x + p_dimensions_1.x < p_translation_2.x ||
        p_translation_1.x > p_translation_2.x + p_dimensions_2.x ||
        p_translation_1.y - p_dimensions_1.y > p_translation_2.y ||
        p_translation_1.y < p_translation_2.y - p_dimensions_2.y
    );

    // If neither has velocity, return the result of the initial AABB check
    if (!p_velocity_1 && !p_velocity_2) return result;

    // Calculate relative velocity
    glm::vec2 velocity1 = p_velocity_1 ? p_velocity_1->GetVelocity() : glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity2 = p_velocity_2 ? p_velocity_2->GetVelocity() : glm::vec2(0.0f, 0.0f);
    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;
    glm::vec2 newTranslation1 = p_translation_1 + relativeVelocity;

    bool newResult = !(
        newTranslation1.x + p_dimensions_1.x < p_translation_2.x ||
        newTranslation1.x > p_translation_2.x + p_dimensions_2.x ||
        newTranslation1.y - p_dimensions_1.y > p_translation_2.y ||
        newTranslation1.y < p_translation_2.y - p_dimensions_2.y
    );

    // Return true if either are true
    return (result || newResult);
}

void ColliderManager::SlideAABB(const glm::vec2& p_translation_1, const glm::vec2& p_translation_2, const glm::vec2& p_dimensions_1, const glm::vec2& p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    // Cache velocities
    glm::vec2 velocity1 = p_velocity_1 ? p_velocity_1->GetVelocity() : glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity2 = p_velocity_2 ? p_velocity_2->GetVelocity() : glm::vec2(0.0f, 0.0f);

    // Early exit if both velocities are zero
    if (velocity1 == glm::vec2(0.0f) && velocity2 == glm::vec2(0.0f))
    {
        return;
    }

    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;

    // Cache the bounds of both objects
    float left1 = p_translation_1.x;
    float right1 = p_translation_1.x + p_dimensions_1.x;
    float top1 = p_translation_1.y;
    float bottom1 = p_translation_1.y - p_dimensions_1.y;

    float left2 = p_translation_2.x;
    float right2 = p_translation_2.x + p_dimensions_2.x;
    float top2 = p_translation_2.y;
    float bottom2 = p_translation_2.y - p_dimensions_2.y;

    // Calculate new positions after applying relative velocity
    float newLeft1 = left1 + relativeVelocity.x;
    float newRight1 = right1 + relativeVelocity.x;
    float newTop1 = top1 + relativeVelocity.y;
    float newBottom1 = bottom1 + relativeVelocity.y;

    glm::vec2 collisionNormal1(0.0f);
    glm::vec2 collisionNormal2(0.0f);

    // Determine the collision normal based on new bounds
    if (newLeft1 <= right2 && left1 > right2)
    {
        collisionNormal2 = glm::vec2(1.0f, 0.0f);  // Left collision
    }
    else if (newRight1 >= left2 && right1 < left2)
    {
        collisionNormal2 = glm::vec2(-1.0f, 0.0f); // Right collision
    }
    else if (newTop1 >= bottom2 && top1 < bottom2)
    {
        collisionNormal2 = glm::vec2(0.0f, -1.0f); // Top collision
    }
    else if (newBottom1 <= top2 && bottom1 > top2)
    {
        collisionNormal2 = glm::vec2(0.0f, 1.0f);  // Bottom collision
    }

    // If no collision normal was determined, exit early
    if (collisionNormal2 == glm::vec2(0.0f))
    {
        return;
    }

    // Calculate the opposite normal for the other object
    collisionNormal1 = -collisionNormal2;

    // Resolve velocities for object 1
    if (p_velocity_1)
    {
        float dotProduct1 = glm::dot(velocity1, collisionNormal2);
        if (dotProduct1 <= 0.0f)
        {
            p_velocity_1->SetVelocity(velocity1 - collisionNormal2 * dotProduct1);
        }
    }

    // Resolve velocities for object 2
    if (p_velocity_2)
    {
        float dotProduct2 = glm::dot(velocity2, collisionNormal1);
        if (dotProduct2 <= 0.0f)
        {
            p_velocity_2->SetVelocity(velocity2 - collisionNormal1 * dotProduct2);
        }
    }
}

bool ColliderManager::IsCollidingInternalUse(ColliderComponent& p_colliderComponent1, ColliderComponent& p_colliderComponent2, float p_delta)
{
    // Grab object pointers
    auto* pObj1 = p_colliderComponent1.GetGameObject();
    auto* pObj2 = p_colliderComponent2.GetGameObject();

    // Grab velocity pointers
    auto* pVel1 = pObj1->GetComponent<VelocityComponent>();
    auto* pVel2 = pObj2->GetComponent<VelocityComponent>();

    // Early out if neither have velocity
    if (!pVel1 && !pVel2) return false;

    // Skip ignored IDs
    if ((p_colliderComponent1.m_IgnoreID == pObj2->GetID()) ||
        (p_colliderComponent2.m_IgnoreID == pObj1->GetID()))
    {
        return false;
    }
    
    // Grab transform pointers
    auto* pTrans1 = pObj1->GetComponent<wolf::Transform2D>();
    auto* pTrans2 = pObj2->GetComponent<wolf::Transform2D>();
    
    glm::vec2 scale1 = p_colliderComponent1.IsRelative() ? pTrans1->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    glm::vec2 scale2 = p_colliderComponent2.IsRelative() ? pTrans2->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    
    glm::vec2 objTranslation1 = pTrans1->GetGlobalPosition();
    glm::vec2 objTranslation2 = pTrans2->GetGlobalPosition();
    
    for(const wolf::Rectangle& collider1 : p_colliderComponent1.GetColliderBoxes())
    { 
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight()) * scale1;
        glm::vec2 offset1 = collider1.GetPosition() * scale1;            
        glm::vec2 translation1 = objTranslation1 + offset1;

        for(const wolf::Rectangle& collider2 : p_colliderComponent2.GetColliderBoxes())
        {                
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight()) * scale2;
            glm::vec2 offset2 = collider2.GetPosition() * scale2;        
            glm::vec2 translation2 = objTranslation2 + offset2;

            if(this->CustomAABBInternalUse(translation1, translation2, dimensions1, dimensions2, pVel1, pVel2, p_delta) > 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool ColliderManager::CustomAABBInternalUse(const glm::vec2& p_translation_1, const glm::vec2& p_translation_2, const glm::vec2& p_dimensions_1, const glm::vec2& p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    // Initial AABB check
    bool result = !(
        p_translation_1.x + p_dimensions_1.x < p_translation_2.x ||
        p_translation_1.x > p_translation_2.x + p_dimensions_2.x ||
        p_translation_1.y - p_dimensions_1.y > p_translation_2.y ||
        p_translation_1.y < p_translation_2.y - p_dimensions_2.y
    );

    // Calculate relative velocity
    glm::vec2 velocity1 = p_velocity_1 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_1->GetVelocity();
    glm::vec2 velocity2 = p_velocity_2 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_2->GetVelocity();
    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;
    glm::vec2 newTranslation1 = p_translation_1 + relativeVelocity;
        
    bool newResult = !(
        newTranslation1.x + p_dimensions_1.x < p_translation_2.x                        ||
        newTranslation1.x                    > p_translation_2.x + p_dimensions_2.x     ||
        newTranslation1.y - p_dimensions_1.y > p_translation_2.y                        ||
        newTranslation1.y                    < p_translation_2.y - p_dimensions_2.y
    );

    // If both were colliding and will continue colliding
    if (result && newResult)
    {
        // TODO: Push overlapping colliders away from each other
        if (p_velocity_1) p_velocity_1->SetVelocity(glm::vec2(0.0f, 0.0f));
        if (p_velocity_2) p_velocity_2->SetVelocity(glm::vec2(0.0f, 0.0f));
        return true;
    }

    // If one is true but the other isn't
    else if (result != newResult)
    {
        this->SlideAABB(p_translation_1, p_translation_2, p_dimensions_1, p_dimensions_2, p_velocity_1, p_velocity_2, p_delta);
        return true;
    }

    // Both results must be false
    return false;
}

void ColliderManager::CheckCornerCollision(float p_delta)
{
    // Precompute moving colliders
    std::vector<std::pair<ColliderComponent*, glm::vec2>> movingColliders;
    std::vector<ColliderComponent*> staticColliders;

    // Separate moving and static colliders
    for (auto&& [id, collider] : m_scene->Each<ColliderComponent>())
    {
        if (!collider.IsActive() || !collider.IsHitbox()) continue;

        // Check velocity
        auto* velocityComponent = collider.GetGameObject()->GetComponent<VelocityComponent>();
        glm::vec2 velocity = velocityComponent ? velocityComponent->GetVelocity() : glm::vec2(0.0f);

        if (velocity != glm::vec2(0.0f))
        {
            movingColliders.push_back({&collider, velocity});
        }
        else
        {
            staticColliders.push_back(&collider);
        }
    }

    // Compare moving colliders with each other and with static colliders
    for (auto& [collider1, velocity1] : movingColliders)
    {
        // Get transform and scale of first collider
        glm::vec2 translation1 = collider1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 scale1 = collider1->IsRelative() ? collider1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale() : glm::vec2(1.0f);

        const bool isSolidEnemy1 = collider1->GetGameObject()->HasAny<MinitaurController, GorgonController>();
        const bool isPlayer1 = collider1->GetGameObject()->HasAny<PlayerController>();

        for (wolf::Rectangle box1 : collider1->GetColliderBoxes())
        {
            auto corners1 = box1.GetCorners();

            // Compare with other moving colliders
            for (auto& [collider2, velocity2] : movingColliders)
            {
                if (collider1 == collider2) continue;
                
                const bool isSolidEnemy2 = collider2->GetGameObject()->HasAny<MinitaurController, GorgonController>();
                const bool isPlayer2 = collider2->GetGameObject()->HasAny<PlayerController>();

                // Skip collisions between enemies to avoid them sticking
                if (isSolidEnemy1 && isSolidEnemy2) continue;

                // Skip collisions between enemies and the player
                if ((isPlayer1 && isSolidEnemy2) || (isPlayer2 && isSolidEnemy1)) continue;

                if (HandleCornerCollision(collider1, velocity1, corners1, collider2, scale1, translation1))
                {
                }
            }

            // Compare with static colliders
            for (auto* collider2 : staticColliders)
            {
                if (collider1 == collider2) continue;

                const bool isSolidEnemy2 = collider2->GetGameObject()->HasAny<MinitaurController, GorgonController>();
                const bool isPlayer2 = collider2->GetGameObject()->HasAny<PlayerController>();

                // Skip collisions between enemies to avoid them sticking
                if (isSolidEnemy1 && isSolidEnemy2) continue;

                // Skip collisions between enemies and the player
                if ((isPlayer1 && isSolidEnemy2) || (isPlayer2 && isSolidEnemy1)) continue;

                if (HandleCornerCollision(collider1, velocity1, corners1, collider2, scale1, translation1))
                {
                }
            }
        }
    }
}

// Helper function to check for and handle corner collisions
bool ColliderManager::HandleCornerCollision(
    ColliderComponent* collider1,
    const glm::vec2& velocity1,
    const std::array<glm::vec2, 4>& corners,
    ColliderComponent* collider2,
    const glm::vec2& scale1,
    const glm::vec2& translation1)
{
    glm::vec2 translation2 = collider2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 scale2 = collider2->IsRelative() ? collider2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale() : glm::vec2(1.0f);

    const float buffer = 0.1f; // Small buffer to push the colliders slightly apart

    // !-- Aurora added this check --!
    // Skip ignored IDs
    if ((collider1->m_IgnoreID == collider2->GetGameObject()->GetID()) ||
        (collider2->m_IgnoreID == collider1->GetGameObject()->GetID()))
    {
        return false;
    }

    for (wolf::Rectangle box2 : collider2->GetColliderBoxes())
    {
        glm::vec2 adjustedTopLeft2 = box2.GetPosition() * scale2 + translation2;
        glm::vec2 adjustedBottomRight2 = adjustedTopLeft2 + glm::vec2(box2.GetWidth(), -box2.GetHeight()) * scale2;

        for (const auto& corner : corners)
        {
            glm::vec2 transformedCorner = corner * scale1 + translation1;

            // Check if the corner is inside the bounds of the second collider
            if (transformedCorner.x >= adjustedTopLeft2.x && transformedCorner.x <= adjustedBottomRight2.x &&
                transformedCorner.y <= adjustedTopLeft2.y && transformedCorner.y >= adjustedBottomRight2.y)
            {
                glm::vec2 pushDirection(0.0f);

                // Determine the smallest axis of penetration
                float overlapX = std::min(
                    std::abs(adjustedBottomRight2.x - transformedCorner.x),
                    std::abs(adjustedTopLeft2.x - transformedCorner.x));
                float overlapY = std::min(
                    std::abs(adjustedTopLeft2.y - transformedCorner.y),
                    std::abs(adjustedBottomRight2.y - transformedCorner.y));

                // Push out based on the smaller overlap
                if (overlapX < overlapY)
                {
                    // Push along the X-axis
                    if (transformedCorner.x > (adjustedTopLeft2.x + adjustedBottomRight2.x) / 2.0f)
                        pushDirection.x = overlapX + buffer; // Push right
                    else
                        pushDirection.x = -(overlapX + buffer); // Push left
                }
                else
                {
                    // Push along the Y-axis
                    if (transformedCorner.y > (adjustedTopLeft2.y + adjustedBottomRight2.y) / 2.0f)
                        pushDirection.y = overlapY + buffer; // Push up
                    else
                        pushDirection.y = -(overlapY + buffer); // Push down
                }

                // Adjust collider1's position to resolve the overlap
                auto* transform = collider1->GetGameObject()->GetComponent<wolf::Transform2D>();
                if (transform)
                {
                    glm::vec2 newPosition = translation1 + pushDirection;
                    transform->SetPosition(newPosition);
                }
                
                // Stop movement in the collision direction
                glm::vec2 newVel = velocity1;
                if (pushDirection.x != 0.0f)
                    newVel.x = 0.0f;
                if (pushDirection.y != 0.0f)
                    newVel.y = 0.0f;

                // Apply the updated velocity back to the velocity component
                auto* velocityComponent = collider1->GetGameObject()->GetComponent<VelocityComponent>();
                if (velocityComponent)
                {
                    velocityComponent->SetVelocity(newVel);
                }

                return true; // Corner collision detected
            }
        }
    }
    return false; // No collision
}

bool ColliderManager::StaticMethodCustomAABB(const glm::vec2& p_translation_1, const glm::vec2& p_translation_2, const glm::vec2& p_dimensions_1, const glm::vec2& p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    // Initial AABB check
    bool result = !(
        p_translation_1.x + p_dimensions_1.x < p_translation_2.x ||
        p_translation_1.x > p_translation_2.x + p_dimensions_2.x ||
        p_translation_1.y - p_dimensions_1.y > p_translation_2.y ||
        p_translation_1.y < p_translation_2.y - p_dimensions_2.y
    );

    // If neither has velocity, return the result of the initial AABB check
    if (!p_velocity_1 && !p_velocity_2) return result;

    // Calculate relative velocity
    glm::vec2 velocity1 = p_velocity_1 ? p_velocity_1->GetVelocity() : glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity2 = p_velocity_2 ? p_velocity_2->GetVelocity() : glm::vec2(0.0f, 0.0f);
    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;
    glm::vec2 newTranslation1 = p_translation_1 + relativeVelocity;

    bool newResult = !(
        newTranslation1.x + p_dimensions_1.x < p_translation_2.x ||
        newTranslation1.x > p_translation_2.x + p_dimensions_2.x ||
        newTranslation1.y - p_dimensions_1.y > p_translation_2.y ||
        newTranslation1.y < p_translation_2.y - p_dimensions_2.y
    );

    // Return true if either are true
    return (result || newResult);
}