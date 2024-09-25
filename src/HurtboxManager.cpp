//-----------------------------------------------------------------------------
// File: HurtboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages Hurtbox collision.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
//-----------------------------------------------------------------------------

#include "HurtboxManager.h"

HurtboxManager::HurtboxManager(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

HurtboxManager::~HurtboxManager()
{
    this->m_scene = nullptr;
}

void HurtboxManager::Update()
{
    this->RemoveFlagged();
    this->CheckCollisions();
}

void HurtboxManager::CheckCollisions()
{
    int i = 0;

    if(HurtboxComponent::s_iComponentCount >= 2)
    {
        for (auto&&[id1, object1, hurtbox1] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>())
        {
            i++;
            for (auto&&[id2, object2, hurtbox2] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>() | std::views::drop(i))
            {

                if(this->IsColliding(&hurtbox1, &hurtbox2)) // If colliders colliding
                {
                    if(hurtbox1.IsHitbox() && hurtbox2.IsHitbox())
                    {
                    }

                    if(hurtbox1.IsHurtboxDamageDealer() && hurtbox2.IsHurtboxDamageReceiver())
                    {
                        HealthComponent* healthComponent = object2.GetComponent<HealthComponent>();
                        if(healthComponent != nullptr)
                        {
                            healthComponent->Damage(hurtbox1.GetDamage());
                        }
                        else
                        {
                            printf("HurtboxManager - Error: HealthComponent not found.\n");
                        }
                    }

                    if(hurtbox1.IsHurtboxDamageReceiver() && hurtbox2.IsHurtboxDamageDealer())
                    {
                        HealthComponent* healthComponent = object1.GetComponent<HealthComponent>();
                        if(healthComponent != nullptr)
                        {
                            healthComponent->Damage(hurtbox2.GetDamage());
                        }
                        else
                        {
                            printf("HurtboxManager - Error: HealthComponent not found.\n");
                        }
                    }

                    if(hurtbox1.IsDestroyedOnCollision())
                    {
                        // std::cout << "HurtboxManager - Delete id1:" << id1 << std::endl;
                        this->m_vToBeDestroyed.push_back(&hurtbox1);
                    }

                    if(hurtbox2.IsDestroyedOnCollision())
                    {
                        // std::cout << "HurtboxManager - Delete id2:" << id2 << std::endl;
                        this->m_vToBeDestroyed.push_back(&hurtbox2);
                    }                    
                }
                
            }
        }
    }  
}

// Remove objects flagged for destruction
void HurtboxManager::RemoveFlagged()
{
    for (int i = 0; i < this->m_vToBeDestroyed.size(); i++)
    {
        std::cout << "HurtboxManager - Remove id:" << this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID() << std::endl;
        this->m_scene->DeleteObject(this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID());
        
    }
    this->m_vToBeDestroyed.clear();
}

bool HurtboxManager::IsColliding(HurtboxComponent* p_hurtboxComponent1, HurtboxComponent* p_hurtboxComponent2)
{
    if(this->IsValidForCollisionCheck(p_hurtboxComponent1, p_hurtboxComponent2))
    {
        for(wolf::Rectangle hurtbox1 : p_hurtboxComponent1->GetHurtboxes())
        {
            glm::vec2 dimensions1 = glm::vec2(hurtbox1.GetWidth(), hurtbox1.GetHeight());
            glm::vec2 offset1 = hurtbox1.GetPosition();

            if(p_hurtboxComponent1->IsRelative()) // If hurtbox component 1 is relative
            {
                glm::vec2 scale1 = p_hurtboxComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions1.x *= scale1.x;
                dimensions1.y *= scale1.y;
                offset1.x *= scale1.x;
                offset1.y *= scale1.y;
            }

            for(wolf::Rectangle hurtbox2 : p_hurtboxComponent2->GetHurtboxes())
            {
                glm::vec2 dimensions2 = glm::vec2(hurtbox2.GetWidth(), hurtbox2.GetHeight());
                glm::vec2 offset2 = hurtbox2.GetPosition();

                if(p_hurtboxComponent2->IsRelative()) // If hurtbox component 2 is relative
                {
                    glm::vec2 scale2 = p_hurtboxComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                    dimensions2.x *= scale2.x;
                    dimensions2.y *= scale2.y;
                    offset2.x *= scale2.x;
                    offset2.y *= scale2.y;
                }


                glm::vec2 translation1 = p_hurtboxComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::vec2 translation2 = p_hurtboxComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

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

bool HurtboxManager::IsValidForCollisionCheck(HurtboxComponent* p_hurtboxComponent1, HurtboxComponent* p_hurtboxComponent2)
{
    if(p_hurtboxComponent1->GetGameObject()->GetID() != p_hurtboxComponent2->GetGameObject()->GetID())              // If not from same object
    {
        if
        (
            (p_hurtboxComponent1->IsHitbox() && p_hurtboxComponent2->IsHitbox()) ||                                 // If are both hitboxes
            (p_hurtboxComponent1->IsHurtboxDamageDealer() && p_hurtboxComponent2->IsHurtboxDamageReceiver()) ||     // If are both damage dealer hurtboxes
            (p_hurtboxComponent1->IsHurtboxDamageReceiver() && p_hurtboxComponent2->IsHurtboxDamageDealer())        // If are both damage receiver hurtboxes
        )
        {
            return true;
        }
    }
    
    return false;
}