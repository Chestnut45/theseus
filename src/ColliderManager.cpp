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
    this->RemoveFlagged();
    this->CheckCollisions(p_delta);
}

// Remove objects flagged for destruction
void ColliderManager::RemoveFlagged()
{
    for (int i = 0; i < this->m_vToBeDestroyed.size(); i++)
    {
        std::cout << "ColliderManager - Remove id:" << this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID() << std::endl;
        this->m_scene->DeleteObject(this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID());
        
    }
    this->m_vToBeDestroyed.clear();
}

// Iterate through collider components to check for collisions
void ColliderManager::CheckCollisions(float p_delta)
{
    int i = 0;

    if(ColliderComponent::s_iComponentCount >= 2)
    {
        for (auto&&[id1, object1, collider1] : this->m_scene->Each<wolf::GameObject, ColliderComponent>())
        {
            i++;
            bool isObject1Mobile = object1.HasAny<VelocityComponent>();
            
            for (auto&&[id2, object2, collider2] : this->m_scene->Each<wolf::GameObject, ColliderComponent>() | std::views::drop(i))
            {
                bool isObject2Mobile = object2.HasAny<VelocityComponent>();

                bool isColliding = false;
                float sweptAABBCollisionTime = 1.0f;
                float sweptAABBRemainingTime = 0.0f;

                if(id1 != id2)
                {
                    // Checking for collision
                    if(collider1.IsHitbox() && collider2.IsHitbox())
                    {
                        if(isObject1Mobile || isObject2Mobile)
                        {
                            if(this->IsColliding(&collider1, &collider2, p_delta))
                            {
                                isColliding = true;
                            } 
                        }   
                    }

                    // else if(
                    //         (collider1.IsHitbox() && collider2.IsHurtbox()) ||
                    //         (collider1.IsHurtbox() && collider2.IsHitbox())
                    // )
                    // {
                    //     if(this->IsColliding(&collider1, &collider2, p_delta))
                    //     {
                    //         isColliding = true;
                    //     }
                    // }

                    else if(collider1.IsHurtboxDamageDealer() && collider2.IsHurtboxDamageReceiver() ||
                            collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
                    {
                        if(this->IsColliding(&collider1, &collider2, p_delta))
                        {
                            isColliding = true;
                        } 
                        
                    }

                    // else if(collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
                    // {
                    //     if(this->IsColliding(&collider1, &collider2, p_delta))
                    //     {
                    //         isColliding = true;
                    //     }    
                    // }

                    // Perform actions if colliding
                    if(isColliding)
                    {
                        if(collider1.IsHitbox() && collider2.IsHitbox())
                        {
                            std::cout << "ColliderManager - Hitboxes Colliding " << object1.GetID() << " - " << object2.GetID() << std::endl;
                            sweptAABBRemainingTime = 1.0f - sweptAABBCollisionTime;
                            glm::vec2 normal1, normal2 = glm::vec2(0.0f, 0.0f);

                            if(isObject1Mobile)
                            {
                                VelocityComponent* velocityComponent = object1.GetComponent<VelocityComponent>();
                                normal1 = velocityComponent->GetNormalisedVelocity();
                                velocityComponent->SetVelocity(glm::vec2(0.0f, 0.0f));
                            }
                            if(isObject2Mobile)
                            {
                                VelocityComponent* velocityComponent = object2.GetComponent<VelocityComponent>();
                                normal2 = velocityComponent->GetNormalisedVelocity();
                                velocityComponent->SetVelocity(glm::vec2(0.0f, 0.0f));
                            }
                        }

                        // if(
                        //     (collider1.IsHitbox() && collider2.IsHurtbox()) ||
                        //     (collider1.IsHurtbox() && collider2.IsHitbox())
                        // )
                        // {
                        //     std::cout << "ColliderManager - Hitbox-Hurtbox Colliding" << std::endl;
                        // }

                        if(collider1.IsHurtboxDamageDealer() && collider2.IsHurtboxDamageReceiver())
                        {
                            std::cout << "ColliderManager - Hurtboxes Colliding" << std::endl;
                            HealthComponent* healthComponent = object2.GetComponent<HealthComponent>();
                            if(healthComponent != nullptr)
                            {
                                healthComponent->Damage(collider1.GetDamage());
                            }
                        }

                        else if(collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
                        {
                            std::cout << "ColliderManager - Hurtboxes Colliding" << std::endl;
                            HealthComponent* healthComponent = object1.GetComponent<HealthComponent>();
                            if(healthComponent != nullptr)
                            {
                                healthComponent->Damage(collider2.GetDamage());
                            }
                        }
                    }
                }                                   
            }
        }
    }  
}

// Iterate through collider boxes of collider components to check for collision
bool ColliderManager::IsColliding(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2, float p_delta)
{
    for(wolf::Rectangle collider1 : p_colliderComponent1->GetColliderBoxes())
    {
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight());
        glm::vec2 offset1 = collider1.GetPosition();

        if(p_colliderComponent1->IsRelative())
        {
            glm::vec2 scale1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            dimensions1 *= scale1;
            offset1 *= scale1;
        }

        glm::vec2 translation1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition() + offset1;
        VelocityComponent* velocity1 = p_colliderComponent1->GetGameObject()->GetComponent<VelocityComponent>();

        for(wolf::Rectangle collider2 : p_colliderComponent2->GetColliderBoxes())
        {
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight());
            glm::vec2 offset2 = collider2.GetPosition();
            VelocityComponent* velocity2 = p_colliderComponent2->GetGameObject()->GetComponent<VelocityComponent>();

            glm::vec2 combinedVelocity = glm::vec2(0.0f, 0.0f);
            if(velocity1 != nullptr)
            {
                combinedVelocity += velocity1->GetVelocity();
            }
            if(velocity2 != nullptr)
            {
                combinedVelocity -= velocity2->GetVelocity();
            }
            
            if(p_colliderComponent2->IsRelative())
            {
                glm::vec2 scale2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions2 *= scale2;
                offset2 *= scale2;
            }
            glm::vec2 translation2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition() + offset2;

            if(combinedVelocity == glm::vec2(0.0f, 0.0f))
            {
                
                if(this->StandardAABB(translation1, translation2, dimensions1, dimensions2))
                {
                    if(
                        p_colliderComponent1->IsDestroyedOnCollision() &&
                        std::find(this->m_vToBeDestroyed.begin(), this->m_vToBeDestroyed.end(), p_colliderComponent1) == this->m_vToBeDestroyed.end()
                        )
                    {

                        this->m_vToBeDestroyed.push_back(p_colliderComponent1);
                    }

                    if(
                        p_colliderComponent2->IsDestroyedOnCollision() &&
                        std::find(this->m_vToBeDestroyed.begin(), this->m_vToBeDestroyed.end(), p_colliderComponent2) == this->m_vToBeDestroyed.end()
                        )
                    {
                        this->m_vToBeDestroyed.push_back(p_colliderComponent2);
                    }

                    return true;
                }
            }
            else
            {
                if(this->StandardAABBBroadphase(translation1, translation2, dimensions1, dimensions2, combinedVelocity * p_delta))
                {
                    float collisionTime = this->SweptAABB(translation1, translation2, dimensions1, dimensions2, combinedVelocity * p_delta);
                    if(collisionTime < 1.0f)
                    {
                        if(p_colliderComponent1->IsDestroyedOnCollision())
                        {
                            this->m_vToBeDestroyed.push_back(p_colliderComponent1);
                        }

                        if(p_colliderComponent2->IsDestroyedOnCollision())
                        {
                            this->m_vToBeDestroyed.push_back(p_colliderComponent2);
                        }

                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ColliderManager::StandardAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2)
{
    return(
        p_translation_1.x + p_dimensions_1.x > p_translation_2.x                        &&  // Right1 > Left2
        p_translation_1.x                    < p_translation_2.x + p_dimensions_2.x     &&  // Left1 < Right2
        p_translation_1.y + p_dimensions_1.y > p_translation_2.y                        &&  // Lower1 > Upper2
        p_translation_1.y                    < p_translation_2.y + p_dimensions_2.y         // Upper1 < Lower2
    );
}

bool ColliderManager::StandardAABBBroadphase(glm::vec2 p_mobile_translation, glm::vec2 p_static_translation, glm::vec2 p_mobile_dimensions, glm::vec2 p_static_dimensions, glm::vec2 p_mobile_velocity)
{
    glm::vec2 mobileBroadphaseTranslation, mobileBroadphaseDimensions;
    mobileBroadphaseTranslation.x = p_mobile_velocity.x > 0.0f ? p_mobile_translation.x : p_mobile_translation.x + (p_mobile_velocity.x);
    mobileBroadphaseTranslation.y = p_mobile_velocity.y > 0.0f ? p_mobile_translation.y : p_mobile_translation.y + (p_mobile_velocity.y);
    mobileBroadphaseDimensions.x = p_mobile_velocity.x > 0.0f ? p_mobile_dimensions.x + (p_mobile_velocity.x) : p_mobile_dimensions.x - (p_mobile_velocity.x);
    mobileBroadphaseDimensions.y = p_mobile_velocity.y > 0.0f ? p_mobile_dimensions.y + (p_mobile_velocity.y) : p_mobile_dimensions.y - (p_mobile_velocity.y);

    return(
        mobileBroadphaseTranslation.x + mobileBroadphaseDimensions.x    >   p_static_translation.x                              &&  // Right1 > Left2
        mobileBroadphaseTranslation.x                                   <   p_static_translation.x + p_static_dimensions.x      &&  // Left1 < Right2
        mobileBroadphaseTranslation.y + mobileBroadphaseDimensions.y    >   p_static_translation.y                              &&  // Lower1 > Upper2
        mobileBroadphaseTranslation.y                                   <   p_static_translation.y + p_static_dimensions.y          // Upper1 < Lower2
    );
}

float ColliderManager::SweptAABB(glm::vec2 p_mobile_translation, glm::vec2 p_static_translation, glm::vec2 p_mobile_dimensions, glm::vec2 p_static_dimensions, glm::vec2 p_mobile_velocity)
{
    float   xEntryDist, yEntryDist, xExitDist, yExitDist,
            xEntryTime, yEntryTime, xExitTime, yExitTime,
            entryTime, exitTime;

    glm::vec2 mobileBroadphaseTranslation, mobileBroadphaseDimensions;
    mobileBroadphaseTranslation.x = p_mobile_velocity.x > 0.0f ? p_mobile_translation.x : p_mobile_translation.x + p_mobile_velocity.x;
    mobileBroadphaseTranslation.y = p_mobile_velocity.y > 0.0f ? p_mobile_translation.y : p_mobile_translation.y + p_mobile_velocity.y;
    mobileBroadphaseDimensions.x = p_mobile_velocity.x > 0.0f ? p_mobile_dimensions.x + p_mobile_velocity.x : p_mobile_dimensions.x - p_mobile_velocity.x;
    mobileBroadphaseDimensions.y = p_mobile_velocity.y > 0.0f ? p_mobile_dimensions.y + p_mobile_velocity.y : p_mobile_dimensions.y - p_mobile_velocity.y;

    // Distance calculations
    int colCaseX, colCaseY = 0;
    if(p_mobile_velocity.x > 0.0f)
    {
        xEntryDist = p_static_translation.x - (p_mobile_translation.x + p_mobile_dimensions.x);
        xExitDist = (p_static_translation.x + p_static_dimensions.x) - p_mobile_translation.x;
        colCaseX = 1;
    }
    else
    {
        xEntryDist = (p_static_translation.x + p_static_dimensions.x) - p_mobile_translation.x;
        xExitDist = p_static_translation.x - (p_mobile_translation.x + p_mobile_dimensions.x);
        colCaseX = 2;
    }

    if (p_mobile_velocity.y > 0)
    {
        yEntryDist = p_static_translation.y - (p_mobile_translation.y + p_mobile_dimensions.y);
        yExitDist = (p_static_translation.y + p_static_dimensions.y) - p_mobile_translation.y;
        colCaseY = 1;
    }
    else
    {
        yEntryDist = (p_static_translation.y + p_static_dimensions.y) - p_mobile_translation.y;
        yExitDist = p_static_translation.y - (p_mobile_translation.y + p_mobile_dimensions.y);
        colCaseY = 2;
    }

    // Time calculations
    if(p_mobile_velocity.x == 0.0f)
    {
        xEntryTime = -std::numeric_limits<float>::infinity();
        xExitTime = std::numeric_limits<float>::infinity();
    }
    else
    {
        xEntryTime = xEntryDist / p_mobile_velocity.x;
        xExitTime = xExitDist / p_mobile_velocity.x;
    }

    if(p_mobile_velocity.y == 0.0f)
    {
        yEntryTime = -std::numeric_limits<float>::infinity();
        yExitTime = std::numeric_limits<float>::infinity();
    }
    else
    {
        yEntryTime = yEntryDist / p_mobile_velocity.y;
        yExitTime = yExitDist / p_mobile_velocity.y;
    }
    entryTime = std::max(xEntryTime, yEntryTime);
    exitTime = std::min(xExitTime, yExitTime);

    // Non-collision check
    if
    (
        (entryTime > exitTime) ||
        (xEntryTime < 0.0f && yEntryTime < 0.0f) ||
        (xEntryTime > 1.0f) ||
        (yEntryTime > 1.0f)
    )
    {
        return 1.0f;
    }
    else
    {   
        return entryTime;
    }
}

float ColliderManager::SweptAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, glm::vec2 p_velocity_1, glm::vec2 p_velocity_2)
{
    float   xEntryDist, yEntryDist, xExitDist, yExitDist,
            xEntryTime, yEntryTime, xExitTime, yExitTime,
            entryTime, exitTime;

    // glm::vec2 broadphaseTranslation1, broadphaseDimensions1;
    // broadphaseTranslation1.x = p_velocity_1.x > 0.0f ? p_translation_1.x : p_translation_1.x + p_velocity_1.x;
    // broadphaseTranslation1.y = p_velocity_1.y > 0.0f ? p_translation_1.y : p_translation_1.y + p_velocity_1.y;
    // broadphaseDimensions1.x = p_velocity_1.x > 0.0f ? p_dimensions_1.x + p_velocity_1.x : p_dimensions_1.x - p_velocity_1.x;
    // broadphaseDimensions1.y = p_velocity_1.y > 0.0f ? p_dimensions_1.y + p_velocity_1.y : p_dimensions_1.y - p_velocity_1.y;

    // // Distance calculations
    // int colCaseX, colCaseY = 0;
    // if(p_mobile_velocity.x > 0.0f)
    // {
    //     xEntryDist = p_translation_2.x - (p_translation_1.x + p_dimensions_1.x);
    //     xExitDist = (p_translation_2.x + p_dimensions_2.x) - p_translation_1.x;
    //     colCaseX = 1;
    // }
    // else
    // {
    //     xEntryDist = (p_translation_2.x + p_dimensions_2.x) - p_translation_1.x;
    //     xExitDist = p_translation_2.x - (p_translation_1.x + p_dimensions_1.x);
    //     colCaseX = 2;
    // }

    // if (p_mobile_velocity.y > 0)
    // {
    //     yEntryDist = p_translation_2.y - (p_translation_1.y + p_dimensions_1.y);
    //     yExitDist = (p_translation_2.y + p_dimensions_2.y) - p_translation_1.y;
    //     colCaseY = 1;
    // }
    // else
    // {
    //     yEntryDist = (p_translation_2.y + p_dimensions_2.y) - p_translation_1.y;
    //     yExitDist = p_translation_2.y - (p_translation_1.y + p_dimensions_1.y);
    //     colCaseY = 2;
    // }

    // // Time calculations
    // if(p_mobile_velocity.x == 0.0f)
    // {
    //     xEntryTime = -std::numeric_limits<float>::infinity();
    //     xExitTime = std::numeric_limits<float>::infinity();
    // }
    // else
    // {
    //     xEntryTime = xEntryDist / p_mobile_velocity.x;
    //     xExitTime = xExitDist / p_mobile_velocity.x;
    // }

    // if(p_mobile_velocity.y == 0.0f)
    // {
    //     yEntryTime = -std::numeric_limits<float>::infinity();
    //     yExitTime = std::numeric_limits<float>::infinity();
    // }
    // else
    // {
    //     yEntryTime = yEntryDist / p_mobile_velocity.y;
    //     yExitTime = yExitDist / p_mobile_velocity.y;
    // }
    // entryTime = std::max(xEntryTime, yEntryTime);
    // exitTime = std::min(xExitTime, yExitTime);

    // // Non-collision check
    // if
    // (
    //     (entryTime > exitTime) ||
    //     (xEntryTime < 0.0f && yEntryTime < 0.0f) ||
    //     (xEntryTime > 1.0f) ||
    //     (yEntryTime > 1.0f)
    // )
    // {
    //     return 1.0f;
    // }
    // else
    // {   
    //     return entryTime;
    // }
    return 1.0f;

}
