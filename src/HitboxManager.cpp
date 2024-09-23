//-----------------------------------------------------------------------------
// File: HitboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
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

// Check all collisions
void HitboxManager::CheckCollisions()
{
    for (auto&&[id1, object1, hitbox1] : this->m_scene->Each<wolf::GameObject, HitboxComponent>())
    {
        for (auto&&[id2, object2, hitbox2] : this->m_scene->Each<wolf::GameObject, HitboxComponent>())
        {
            if(id1 != id2)
            {
                if(this->IsColliding(&hitbox1, &hitbox2))
                {
                    this->count++;
                    std::cout << "HitBoxCollide" << this->count << std::endl;
                    //-------------------------------------------//
                    //                                           //
                    // DO THING - DO THING - DO THING - DO THING //
                    // DO THING - DO THING - DO THING - DO THING //
                    // DO THING - DO THING - DO THING - DO THING //
                    //                                           //
                    //-------------------------------------------//
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