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
    for (auto&&[id, object, hitbox] : this->m_scene->Each<wolf::GameObject, HitboxComponent>())
    {
        if(hitbox.IsToBeDestroyed())
        {
            this->m_scene->DeleteObject(id);
        }
    }
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
                        std::cout << "HitBoxCollide" << std::endl;

                        if(!hitbox1.IsDestroyedOnCollision() && !hitbox2.IsDestroyedOnCollision())
                        {
                            //----------//
                            //          //
                            // DO THING //
                            //          //
                            //----------//
                        }

                        else
                        {
                            if(hitbox1.IsDestroyedOnCollision())
                            {
                                std::cout << "Delete hitbox id1:" << id1 << std::endl;
                                hitbox1.RaiseDestroyFlag();
                            }

                            if(hitbox2.IsDestroyedOnCollision())
                            {
                                std::cout << "Delete hitbox id2:" << id2 << std::endl;
                                hitbox2.RaiseDestroyFlag();
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
    glm::vec2 dimension1 = p_hitbox1->GetDimensions();
    glm::vec2 dimension2 = p_hitbox2->GetDimensions();
    glm::vec2 position1 = p_hitbox1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 position2 = p_hitbox2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if(
        position1.x + dimension1.x > position2.x && // Right1 > Left2
        position1.x < position2.x + dimension2.x && // Left1 < Right2
        position1.y + dimension1.y > position2.y && // Lower1 > Upper2
        position1.y < position2.y + dimension2.y    // Upper1 < Lower2
        )
    {
        return true;
    }
    return false;
}