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

void ColliderManager::Update()
{
    this->RemoveFlagged();
    this->CheckCollisions();
}

void ColliderManager::CheckCollisions()
{
    int i = 0;

    if(ColliderComponent::s_iComponentCount >= 2)
    {
        for (auto&&[id1, object1, collider1] : this->m_scene->Each<wolf::GameObject, ColliderComponent>())
        {
            i++;
            for (auto&&[id2, object2, collider2] : this->m_scene->Each<wolf::GameObject, ColliderComponent>() | std::views::drop(i))
            {

                if(collider1.IsHitbox() && collider2.IsHitbox())
                {
                    bool isObject1Mobile = object1.HasAny<VelocityComponent>();
                    bool isObject2Mobile = object2.HasAny<VelocityComponent>();

                    if(isObject1Mobile && isObject2Mobile)
                    {
                        if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                        {
                            std::cout << "ColliderManager - Hitboxes Colliding" << std::endl;
                        } 
                    }
                    
                    else if(!isObject1Mobile && isObject2Mobile)
                    {
                        if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                        {
                            std::cout << "ColliderManager - Hitboxes Colliding" << std::endl;
                        }   
                    }

                    else if(isObject1Mobile && !isObject2Mobile)
                    {
                        if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                        {
                            std::cout << "ColliderManager - Hitboxes Colliding" << std::endl;
                        }   
                    }

                    else
                    {

                    }
                }

                if(collider1.IsHurtboxDamageDealer() && collider2.IsHurtboxDamageReceiver())
                {
                    if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                    {
                        std::cout << "ColliderManager - Hurtboxes Colliding" << std::endl;
                        HealthComponent* healthComponent = object2.GetComponent<HealthComponent>();
                        if(healthComponent != nullptr)
                        {
                            healthComponent->Damage(collider1.GetDamage());
                        }
                        else
                        {
                            printf("ColliderManager - Error: HealthComponent not found.\n");
                        }
                    } 
                    
                }

                else if(collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
                {
                    if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                    {
                        std::cout << "ColliderManager - Hurtboxes Colliding" << std::endl;
                        HealthComponent* healthComponent = object1.GetComponent<HealthComponent>();
                        if(healthComponent != nullptr)
                        {
                            healthComponent->Damage(collider2.GetDamage());
                        }
                        else
                        {
                            printf("ColliderManager - Error: HealthComponent not found.\n");
                        }
                    }    
                }                             
            }
        }
    }  
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

// Check collision for two collider components
bool ColliderManager::IsColliding(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2)
{
    for(wolf::Rectangle collider1 : p_colliderComponent1->GetColliderBoxes())
    {
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight());
        glm::vec2 offset1 = collider1.GetPosition();
        glm::vec2 translation1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        if(p_colliderComponent1->IsRelative())
        {
            glm::vec2 scale1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            dimensions1 *= scale1;
            offset1 *= scale1;
        }

        for(wolf::Rectangle collider2 : p_colliderComponent2->GetColliderBoxes())
        {
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight());
            glm::vec2 offset2 = collider2.GetPosition();
            glm::vec2 translation2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

            if(p_colliderComponent2->IsRelative())
            {
                glm::vec2 scale2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions2 *= scale2;
                offset2 *= scale2;
            }

            if(
                translation1.x + dimensions1.x + offset1.x > translation2.x + offset2.x                    && // Right1 > Left2
                translation1.x + offset1.x                 < translation2.x + dimensions2.x + offset2.x    && // Left1 < Right2
                translation1.y + dimensions1.y + offset1.y > translation2.y + offset2.y                    && // Lower1 > Upper2
                translation1.y + offset1.y                 < translation2.y + dimensions2.y + offset2.y       // Upper1 < Lower2
                )
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
    return false;
}

bool ColliderManager::IsSweptAABBColliding(ColliderComponent* p_mobile_collider, ColliderComponent* p_static_collider)
{
    for(wolf::Rectangle mobileCollider : p_mobile_collider->GetColliderBoxes())
    {
        glm::vec2 mobileDimensions = glm::vec2(mobileCollider.GetWidth(), mobileCollider.GetHeight());
        glm::vec2 mobileOffset = mobileCollider.GetPosition();
        glm::vec2 mobileTranslation = p_mobile_collider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        if(p_mobile_collider->IsRelative())
        {
            glm::vec2 mobileScale = p_mobile_collider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            mobileDimensions *= mobileScale;
            mobileOffset *= mobileScale;        
        }

        for(wolf::Rectangle staticCollider : p_static_collider->GetColliderBoxes())
        {
            glm::vec2 staticDimensions = glm::vec2(staticCollider.GetWidth(), staticCollider.GetHeight());
            glm::vec2 staticOffset = staticCollider.GetPosition();
            glm::vec2 staticTranslation = p_static_collider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

            if(p_static_collider->IsRelative())
            {
                glm::vec2 staticScale = p_static_collider->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                staticDimensions *= staticScale;
                staticOffset *= staticScale;
            }
        }
    }

    return false;
}

bool ColliderManager::IsValidForCollisionCheck(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2)
{
    if(p_colliderComponent1->GetGameObject()->GetID() != p_colliderComponent2->GetGameObject()->GetID())              // If not from same object
    {
        if
        (
            (p_colliderComponent1->IsHitbox() && p_colliderComponent2->IsHitbox()) ||                                 // If are both hitboxes
            (p_colliderComponent1->IsHurtboxDamageDealer() && p_colliderComponent2->IsHurtboxDamageReceiver()) ||     // If are both damage dealer hurtboxes
            (p_colliderComponent1->IsHurtboxDamageReceiver() && p_colliderComponent2->IsHurtboxDamageDealer())        // If are both damage receiver hurtboxes
        )
        {
            return true;
        }
    }
    
    return false;
}