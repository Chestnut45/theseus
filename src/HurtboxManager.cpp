//-----------------------------------------------------------------------------
// File: HurtboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
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

void HurtboxManager::CheckCollisions()
{
for (auto&&[id1, object1, hurtbox1] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>())
    {
        for (auto&&[id2, object2, hurtbox2] : this->m_scene->Each<wolf::GameObject, HurtboxComponent>())
        {
            if(id1 != id2)
            {
                if(this->IsColliding(&hurtbox1, &hurtbox2))
                {
                    this->count++;
                    std::cout << "HurtBoxCollide" << this->count << std::endl;                    
                }
            }
        }
    }
}

bool HurtboxManager::IsColliding(HurtboxComponent* p_hurtbox1, HurtboxComponent* p_hurtbox2)
{
    if(p_hurtbox1->GetType() != p_hurtbox2->GetType())
    {
        glm::vec2 dimension1 = p_hurtbox1->GetDimensions();
        glm::vec2 dimension2 = p_hurtbox2->GetDimensions();
        glm::vec2 position1 = p_hurtbox1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::vec2 position2 = p_hurtbox2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        if(
            position1.x + dimension1.x > position2.x && // Right1 > Left2
            position1.x < position2.x + dimension2.x && // Left1 < Right2
            position1.y + dimension1.y > position2.y && // Lower1 > Upper2
            position1.y < position2.y + dimension2.y    // Upper1 < Lower2
            )
        {
            HealthComponent* healthComponent = nullptr;
            if(p_hurtbox1->GetType() == 0)
            {
                healthComponent = p_hurtbox1->GetGameObject()->GetComponent<HealthComponent>();  
            }

            else if(p_hurtbox2->GetType() == 0)
            {
                healthComponent = p_hurtbox2->GetGameObject()->GetComponent<HealthComponent>();
            }

            if(healthComponent!= nullptr)
            {
                healthComponent->Damage(1);
            }

            return true;
        }
    }
    
    return false;
}