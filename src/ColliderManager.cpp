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

                if(this->IsColliding(&collider1, &collider2)) // If colliders colliding
                {
                    if(collider1.IsHitbox() && collider2.IsHitbox())
                    {
                        std::cout << "ColliderManager - Hitboxes Colliding" << std::endl;
                    }

                    if(collider1.IsHurtboxDamageDealer() && collider2.IsHurtboxDamageReceiver())
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
                    else if(collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
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

                    if(collider1.IsDestroyedOnCollision())
                    {
                        this->m_vToBeDestroyed.push_back(&collider1);
                    }

                    if(collider2.IsDestroyedOnCollision())
                    {
                        this->m_vToBeDestroyed.push_back(&collider2);
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

bool ColliderManager::IsColliding(ColliderComponent* p_colliderComponent1, ColliderComponent* p_colliderComponent2)
{
    if(this->IsValidForCollisionCheck(p_colliderComponent1, p_colliderComponent2))
    {
        for(wolf::Rectangle collider1 : p_colliderComponent1->GetColliderBoxes())
        {
            glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight());
            glm::vec2 offset1 = collider1.GetPosition();

            if(p_colliderComponent1->IsRelative())
            {
                glm::vec2 scale1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions1.x *= scale1.x;
                dimensions1.y *= scale1.y;
                offset1.x *= scale1.x;
                offset1.y *= scale1.y;
            }

            for(wolf::Rectangle collider2 : p_colliderComponent2->GetColliderBoxes())
            {
                glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight());
                glm::vec2 offset2 = collider2.GetPosition();

                if(p_colliderComponent2->IsRelative())
                {
                    glm::vec2 scale2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                    dimensions2.x *= scale2.x;
                    dimensions2.y *= scale2.y;
                    offset2.x *= scale2.x;
                    offset2.y *= scale2.y;
                }


                glm::vec2 translation1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::vec2 translation2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

                if(
                    translation1.x + dimensions1.x + offset1.x > translation2.x + offset2.x                    && // Right1 > Left2
                    translation1.x + offset1.x                 < translation2.x + dimensions2.x + offset2.x    && // Left1 < Right2
                    translation1.y + dimensions1.y + offset1.y > translation2.y + offset2.y                    && // Lower1 > Upper2
                    translation1.y + offset1.y                 < translation2.y + dimensions2.y + offset2.y       // Upper1 < Lower2
                    )
                {
                    return true;
                }

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