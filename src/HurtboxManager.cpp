//-----------------------------------------------------------------------------
// File: HurtboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages Hurtbox collision.
//-----------------------------------------------------------------------------

#include "HurtboxManager.h"

HurtboxManager::HurtboxManager()
{
}

HurtboxManager::~HurtboxManager()
{
    this->m_scene = nullptr;
}

void HurtboxManager::Init(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

void HurtboxManager::Update()
{
    this->RemoveFlagged();
    this->CheckCollisions();
}

void HurtboxManager::CheckCollisions()
{
    int i = 0;
    for (auto&&[id1, object1, hurtbox1] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>())
    {
        i++;
        for (auto&&[id2, object2, hurtbox2] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>() | std::views::drop(i))
        {
            if(id1 != id2)
            {
                if(this->IsColliding(&hurtbox1, &hurtbox2))
                {
                    //std::cout << "HurtBoxCollide" << this->count << std::endl;

                HealthComponent* healthComponent = nullptr;
            
                if(hurtbox1.GetType() == 0)
                {
                    healthComponent = object1.GetComponent<HealthComponent>();  
                    if(healthComponent!= nullptr)
                    {
                        healthComponent->Damage(hurtbox2.GetDamage());
                    }
                }

                else if(hurtbox2.GetType() == 0)
                {
                    healthComponent = hurtbox2.GetGameObject()->GetComponent<HealthComponent>();
                    if(healthComponent!= nullptr)
                    {
                        healthComponent->Damage(hurtbox1.GetDamage());
                    }
                }
                std::cout << "HurtBoxCollide - health:" << healthComponent->GetHealth() << std::endl;


                    if(hurtbox1.IsDestroyedOnCollision())
                        {
                            std::cout << "Delete hurtbox id1:" << id1 << std::endl;
                            hurtbox1.RaiseDestroyFlag();
                            //this->m_scene->DeleteObject(id1);
                            //-------------------------------------//
                            //                                     //
                            // DESTROY GAME OBJECT - DELAY 1 FRAME //
                            //                                     //
                            //-------------------------------------//
                        }

                        if(hurtbox2.IsDestroyedOnCollision())
                        {
                            std::cout << "Delete hurtbox id2:" << id2 << std::endl;
                            hurtbox2.RaiseDestroyFlag();
                            //this->m_scene->DeleteObject(id2);
                            //-------------------------------------//
                            //                                     //
                            // DESTROY GAME OBJECT - DELAY 1 FRAME //
                            //                                     //
                            //-------------------------------------//
                        }                    
                    }
                }
        }
    }
}

// Remove objects flagged for destruction
void HurtboxManager::RemoveFlagged()
{
    for (auto&&[id, object, hurtbox] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>())
    {
        if(hurtbox.IsToBeDestroyed())
        {
            this->m_scene->DeleteObject(id);
        }
    }
}

bool HurtboxManager::IsColliding(HurtboxComponent* p_hurtboxComponent1, HurtboxComponent* p_hurtboxComponent2)
{
    if(p_hurtboxComponent1->GetType() != p_hurtboxComponent2->GetType())
    {
        glm::vec2 dimension1 = p_hurtboxComponent1->GetDimensions();
        glm::vec2 dimension2 = p_hurtboxComponent2->GetDimensions();
        glm::vec2 position1 = p_hurtboxComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 position2 = p_hurtboxComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        if(
            position1.x + dimension1.x > position2.x && // Right1 > Left2
            position1.x < position2.x + dimension2.x && // Left1 < Right2
            position1.y + dimension1.y > position2.y && // Lower1 > Upper2
            position1.y < position2.y + dimension2.y    // Upper1 < Lower2
            )
        {
            return true;
        }
    }
    
    return false;
}