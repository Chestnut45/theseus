//-----------------------------------------------------------------------------
// File: ColliderManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages collider collisions.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
//-----------------------------------------------------------------------------

#include "ColliderManager.h"

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
    this->CheckCollisions(p_delta);
    this->RemoveFlagged();
}

// Remove objects flagged for destruction
void ColliderManager::RemoveFlagged()
{
    for (int i = 0; i < this->m_vToBeDestroyed.size(); i++)
    {
        // std::cout << "ColliderManager - Remove id:" << this->m_vToBeDestroyed.at(i) << std::endl;
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

        // Cache properties for collider1
        const bool isHitbox1 = collider1.IsHitbox();
        const bool isDestroyedOnCollision1 = collider1.IsDestroyedOnCollision();

        // Iterate through remaining colliders after i
        for (auto&& [id2, collider2] : this->m_scene->Each<ColliderComponent>() | std::views::drop(i))
        {

            // Skip inactive or flagged colliders
            if (!collider2.IsActive()) continue;

            // Cache properties for collider2
            const bool isHitbox2 = collider2.IsHitbox();
            const bool isDestroyedOnCollision2 = collider2.IsDestroyedOnCollision();

            // Collision check conditions
            bool isCollisionCheckRequired =
                (isHitbox1 && isHitbox2) ||
                isDestroyedOnCollision1 ||
                isDestroyedOnCollision2;

            if (!isCollisionCheckRequired) continue;

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