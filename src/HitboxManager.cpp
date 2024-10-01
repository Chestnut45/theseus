//-----------------------------------------------------------------------------
// File: HitboxManager.cpp
// Original Author: Nguyễn Minh Nhật
// Manages Hitbox collision.
// User Guide:
//     + Create new Manager object before any game object is added to scene
//     + Init() to pass reference to scene
//     + Call Update() every frame
// Notes:
//     + s_iComponentCount incremented/decremented in HitboxComponent
//-----------------------------------------------------------------------------

#include "HitboxManager.h"

// Constructor
HitboxManager::HitboxManager(wolf::Scene* p_scene)
{
    this->m_scene = p_scene;
}

// Destructor
HitboxManager::~HitboxManager()
{
 this->m_scene = nullptr;
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
    }
    this->m_vToBeDestroyed.clear();
}
bool HitboxManager::IsColliding(HitboxComponent* p_hitboxComponent, HurtboxComponent* p_hurtboxComponent)
{
    if (!p_hitboxComponent || !p_hurtboxComponent)
        return false;

    glm::vec2 hitboxPosition = p_hitboxComponent->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 hurtboxPosition = p_hurtboxComponent->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for (wolf::Rectangle hitbox : p_hitboxComponent->GetHitboxes())
    {
        glm::vec2 dimensions1 = glm::vec2(hitbox.GetWidth(), hitbox.GetHeight());
        glm::vec2 offset1 = hitbox.GetPosition();

        if (p_hitboxComponent->IsRelative())
        {
            glm::vec2 scale1 = p_hitboxComponent->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            dimensions1.x *= scale1.x;
            dimensions1.y *= scale1.y;
            offset1.x *= scale1.x;
            offset1.y *= scale1.y;
        }

        for (wolf::Rectangle hurtbox : p_hurtboxComponent->GetHurtboxes())
        {
            glm::vec2 dimensions2 = glm::vec2(hurtbox.GetWidth(), hurtbox.GetHeight());
            glm::vec2 offset2 = hurtbox.GetPosition();

            if (p_hurtboxComponent->IsRelative())
            {
                glm::vec2 scale2 = p_hurtboxComponent->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions2.x *= scale2.x;
                dimensions2.y *= scale2.y;
                offset2.x *= scale2.x;
                offset2.y *= scale2.y;
            }

            // Check for collision between hitbox and hurtbox
            if (
                hitboxPosition.x + dimensions1.x + offset1.x > hurtboxPosition.x + offset2.x                   && // Right1 > Left2
                hitboxPosition.x + offset1.x                 < hurtboxPosition.x + dimensions2.x + offset2.x   && // Left1 < Right2
                hitboxPosition.y + dimensions1.y + offset1.y > hurtboxPosition.y + offset2.y                   && // Lower1 > Upper2
                hitboxPosition.y + offset1.y                 < hurtboxPosition.y + dimensions2.y + offset2.y      // Upper1 < Lower2
                )
            {
                return true;
            }
        }
    }
    return false;
}
// Check all collisions
void HitboxManager::CheckCollisions()
{
    int i = 0;

    if(HitboxComponent::s_iComponentCount >= 2)
    {
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
                                    // std::cout << "HitboxManager - Delete id1:" << id1 << std::endl;
                                    this->m_vToBeDestroyed.push_back(&hitbox1);
                                }

                                if(hitbox2.IsDestroyedOnCollision())
                                {
                                    // std::cout << "HitboxManager - Delete id2:" << id2 << std::endl;
                                    this->m_vToBeDestroyed.push_back(&hitbox2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Check collisions for 2 boxes
bool HitboxManager::IsColliding(HitboxComponent* p_hitboxComponent1, HitboxComponent* p_hitboxComponent2)
{
    glm::vec2 position1 = p_hitboxComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 position2 = p_hitboxComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    for(wolf::Rectangle hitbox1 : p_hitboxComponent1->GetHitboxes())
    {
        glm::vec2 dimensions1 = glm::vec2(hitbox1.GetWidth(), hitbox1.GetHeight());
        glm::vec2 offset1 = hitbox1.GetPosition();

        if(p_hitboxComponent1->IsRelative())
        {
            glm::vec2 scale1 = p_hitboxComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            dimensions1.x *= scale1.x;
            dimensions1.y *= scale1.y;
            offset1.x *= scale1.x;
            offset1.y *= scale1.y;
        }

        for(wolf::Rectangle hitbox2 : p_hitboxComponent2->GetHitboxes())
        {
            glm::vec2 dimensions2 = glm::vec2(hitbox2.GetWidth(), hitbox2.GetHeight());
            glm::vec2 offset2 = hitbox2.GetPosition();
            
            if(p_hitboxComponent2->IsRelative())
            {
                glm::vec2 scale2 = p_hitboxComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale();
                dimensions2.x *= scale2.x;
                dimensions2.y *= scale2.y;
                offset2.x *= scale2.x;
                offset2.y *= scale2.y;
            }
            
            if(
                position1.x + dimensions1.x + offset1.x > position2.x + offset2.x                   && // Right1 > Left2
                position1.x + offset1.x                 < position2.x + dimensions2.x + offset2.x   && // Left1 < Right2
                position1.y + dimensions1.y + offset1.y > position2.y + offset2.y                   && // Lower1 > Upper2
                position1.y + offset1.y                 < position2.y + dimensions2.y + offset2.y      // Upper1 < Lower2
                )
            {
                return true;
            }        
        }
    }
    return false;
}