//-----------------------------------------------------------------------------
// File: HitboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages Hitbox collision.
// feat. D. Landry
//-----------------------------------------------------------------------------

#include "HitboxManager.h"

// Constructor
HitboxManager::HitboxManager()
{

}

// Destructor
HitboxManager::~HitboxManager()
{
 this->m_scene = nullptr;
}

// Init hitbox manager
void HitboxManager::Init(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

void HitboxManager::Update()
{
    this->RemoveFlagged();
    this->CheckCollisions();
}

// Remove objects flagged for destruction
void HitboxManager::RemoveFlagged()
{

    for(int i = 0; i < this->m_vToBeDestroyed.size(); i++)
    {
        std::cout << "HitboxManager - Remove id:" << this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID() << std::endl;
        this->m_scene->DeleteObject(this->m_vToBeDestroyed.at(i)->GetGameObject()->GetID());
        std::cout << "HitboxManager - NOC:" << HitboxComponent::GetComponentCount() << std::endl;
    }
    this->m_vToBeDestroyed.clear();
}

// Check all collisions
void HitboxManager::CheckCollisions()
{
    int i = 0;
    for (auto&&[id1, object1, hitbox1] : this->m_scene->Each<wolf::GameObject, HitboxComponent>())
    {
        i++;
        for (auto&&[id2, object2, hitbox2] : this->m_scene->Each<wolf::GameObject, HitboxComponent>() | std::views::drop(i))
        {
            if(id1 != id2)
            {
                // Check if any object is mobile
                if(
                    object1.HasAny<VelocityComponent>() ||
                    object2.HasAny<VelocityComponent>()
                )
                {
                    if(this->IsColliding(&hitbox1, &hitbox2))
                    {
                        //std::cout << "HitBoxCollide" << std::endl;

                        // Case: both indestructible on collision
                        if(!hitbox1.IsDestroyedOnCollision() && !hitbox2.IsDestroyedOnCollision())
                        {
                            //----------//
                            //          //
                            // DO THING //
                            //          //
                            //----------//
                        }

                        // Case: one or both destructible on collision
                        else
                        {
                            if(hitbox1.IsDestroyedOnCollision())
                            {
                                std::cout << "HitboxManager - Delete id1:" << id1 << std::endl;
                                hitbox1.RaiseDestroyFlag();
                                this->m_vToBeDestroyed.push_back(&hitbox1);
                            }

                            if(hitbox2.IsDestroyedOnCollision())
                            {
                                std::cout << "HitboxManager - Delete id2:" << id2 << std::endl;
                                hitbox2.RaiseDestroyFlag();
                                this->m_vToBeDestroyed.push_back(&hitbox2);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Check collisions for 2 boxes
bool HitboxManager::IsColliding(HitboxComponent* p_hitbox1, HitboxComponent* p_hitbox2)
{
    glm::vec2 position1 = p_hitbox1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 position2 = p_hitbox2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for(wolf::Rectangle hitbox1 : p_hitbox1->GetHitboxes())
    {
        glm::vec2 dimensions1 = glm::vec2(hitbox1.GetWidth(), hitbox1.GetHeight());
        for(wolf::Rectangle hitbox2 : p_hitbox2->GetHitboxes())
        {
            glm::vec2 dimensions2 = glm::vec2(hitbox2.GetWidth(), hitbox2.GetHeight());
            if(
                position1.x + dimensions1.x > position2.x && // Right1 > Left2
                position1.x < position2.x + dimensions2.x && // Left1 < Right2
                position1.y + dimensions1.y > position2.y && // Lower1 > Upper2
                position1.y < position2.y + dimensions2.y    // Upper1 < Lower2
                )
            {
                return true;
            }        
        }
    }
    return false;
}