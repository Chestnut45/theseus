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
        std::cout << "ColliderManager - Remove id:" << this->m_vToBeDestroyed.at(i) << std::endl;
        this->m_scene->DeleteObject(this->m_vToBeDestroyed.at(i));
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
            VelocityComponent* velocityComponent1 = object1.GetComponent<VelocityComponent>();

            for (auto&&[id2, object2, collider2] : this->m_scene->Each<wolf::GameObject, ColliderComponent>() | std::views::drop(i))
            {
                VelocityComponent* velocityComponent2 = object2.GetComponent<VelocityComponent>();

                // Checking for collision
                if (
                    (collider1.IsHitbox() && collider2.IsHitbox())                              || 
                    (collider1.IsHurtboxDamageDealer() && collider2.IsHurtboxDamageReceiver())  ||
                    (collider1.IsHurtboxDamageReceiver() && collider2.IsHurtboxDamageDealer())
                    )
                {
                    if(this->IsColliding(&collider1, &collider2, p_delta))
                    {
                        if(!collider1.m_bIsFlaggedForDestruction && collider1.IsDestroyedOnCollision())
                        {
                            collider1.m_bIsFlaggedForDestruction = true;
                            this->m_vToBeDestroyed.push_back(id1);
                        }

                        if(!collider2.m_bIsFlaggedForDestruction && collider2.IsDestroyedOnCollision())
                        {
                            collider2.m_bIsFlaggedForDestruction = true;
                            this->m_vToBeDestroyed.push_back(id2);
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
    if(
        (p_colliderComponent1->GetGameObject()->GetID() == p_colliderComponent2->GetGameObject()->GetID())  ||
        (p_colliderComponent1->m_IgnoreID == p_colliderComponent2->GetGameObject()->GetID())                || 
        (p_colliderComponent2->m_IgnoreID == p_colliderComponent1->GetGameObject()->GetID())
    )
    {
        return false;
    }

    glm::vec2 scale1 = p_colliderComponent1->IsRelative() ? p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
    glm::vec2 objTranslation1 = p_colliderComponent1->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    VelocityComponent* velocity1 = p_colliderComponent1->GetGameObject()->GetComponent<VelocityComponent>();
    
    for(const wolf::Rectangle& collider1 : p_colliderComponent1->GetColliderBoxes())
    { 
        glm::vec2 dimensions1 = glm::vec2(collider1.GetWidth(), collider1.GetHeight()) * scale1;
        glm::vec2 offset1 = collider1.GetPosition() * scale1;            
        glm::vec2 translation1 = objTranslation1 + offset1;

        glm::vec2 scale2 = p_colliderComponent2->IsRelative() ? p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalScale() : glm::vec2(1.0f, 1.0f);
        glm::vec2 objTranslation2 = p_colliderComponent2->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        VelocityComponent* velocity2 = p_colliderComponent2->GetGameObject()->GetComponent<VelocityComponent>();
        
        for(const wolf::Rectangle& collider2 : p_colliderComponent2->GetColliderBoxes())
        {                
            glm::vec2 dimensions2 = glm::vec2(collider2.GetWidth(), collider2.GetHeight()) * scale2;
            glm::vec2 offset2 = collider2.GetPosition() * scale2;        
            glm::vec2 translation2 = objTranslation2 + offset2;

            if(this->StandardAABBWithSliding(translation1, translation2, dimensions1, dimensions2, velocity1, velocity2, p_delta))
            {
                return true;
            }
        }
    }
    return false;
}

bool ColliderManager::StandardAABBWithSliding(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    glm::vec2 velocity1 = p_velocity_1 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_1->GetVelocity();
    glm::vec2 velocity2 = p_velocity_2 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_2->GetVelocity();

    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;

    bool result = !(
        p_translation_1.x + p_dimensions_1.x < p_translation_2.x                        ||
        p_translation_1.x                    > p_translation_2.x + p_dimensions_2.x     ||
        p_translation_1.y - p_dimensions_1.y > p_translation_2.y                        ||
        p_translation_1.y                    < p_translation_2.y - p_dimensions_2.y
    );    

    // If both hitboxes, respond accordingly
    if
    (
        p_velocity_1->GetGameObject()->GetComponent<ColliderComponent>()->IsHitbox()    && 
        p_velocity_2->GetGameObject()->GetComponent<ColliderComponent>()->IsHitbox()
    )
    {
        glm::vec2 newTranslation1 = p_translation_1 + relativeVelocity;
        
        bool newResult = !(
        newTranslation1.x + p_dimensions_1.x < p_translation_2.x                        ||
        newTranslation1.x                    > p_translation_2.x + p_dimensions_2.x     ||
        newTranslation1.y - p_dimensions_1.y > p_translation_2.y                        ||
        newTranslation1.y                    < p_translation_2.y - p_dimensions_2.y
        );

        // Collision cases
        if(result || newResult)
        {
            // Respond only if a VelocityComponent is available
            if
            (
                p_velocity_1 != nullptr ||
                p_velocity_2 != nullptr
            )
            {
                if(result && newResult)
                {
                    printf("ColliderManager - 11\n");  
                    p_velocity_1->SetVelocity(glm::vec2(0.0f, 0.0f));
                    p_velocity_2->SetVelocity(glm::vec2(0.0f, 0.0f));
                }

                else if (!result && newResult)
                {
                    printf("ColliderManager - 01\n");
                    this->SlideAABB(p_translation_1, p_translation_2, p_dimensions_1, p_dimensions_2, p_velocity_1, p_velocity_2, p_delta);
                }

                else if(result && !newResult)
                {
                    printf("ColliderManager - 10\n");
                    this->SlideAABB(p_translation_1, p_translation_2, p_dimensions_1, p_dimensions_2, p_velocity_1, p_velocity_2, p_delta);
                }
            }
            return true;
        }
    }
    // If not, return standard AABB
    else
    {
        return result;
    }

    // No collision
    return false;
}

void ColliderManager::SlideAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
{
    glm::vec2 velocity1 = p_velocity_1 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_1->GetVelocity();
    glm::vec2 velocity2 = p_velocity_2 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_2->GetVelocity();
    
    glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;
    
    float   newLeft1, newRight1,newTop1, newBottom1,
            left1, right1, top1, bottom1,
            left2, right2, top2, bottom2;

    left1 = p_translation_1.x;
    right1 = p_translation_1.x + p_dimensions_1.x;
    top1 = p_translation_1.y;
    bottom1 = p_translation_1.y - p_dimensions_1.y;

    newLeft1 = left1 + relativeVelocity.x;
    newRight1 = right1 + relativeVelocity.x;
    newTop1 = top1 + relativeVelocity.y;
    newBottom1 = bottom1 + relativeVelocity.y;

    left2 = p_translation_2.x;
    right2 = p_translation_2.x + p_dimensions_2.x;
    top2 = p_translation_2.y;
    bottom2 = p_translation_2.y - p_dimensions_2.y;

    glm::vec2 collisionNormal1 = glm::vec2(0.0f, 0.0f);
    glm::vec2 collisionNormal2 = glm::vec2(0.0f, 0.0f);

    // obj1 left collision
    if(newLeft1 <= right2 && left1 > right2)
    {
        collisionNormal2 = glm::normalize(glm::vec2(1.0f, 0.0f));
        // printf("ColliderManager - C1\n");
    }

    // obj1 right collision
    else if(newRight1 >= left2 && right1 < left2)
    {
        collisionNormal2 = glm::normalize(glm::vec2(-1.0f, 0.0f));
        // printf("ColliderManager - C2\n");
    }

    // obj1 top collision
    else if(newTop1 >= bottom2 && top1 < bottom2)
    {
        collisionNormal2 = glm::normalize(glm::vec2(0.0f, -1.0f));
        // printf("ColliderManager - C3\n");
    }

    // obj1 bottom collision
    else if(newBottom1 <= top2 && bottom1 > top2)
    {
        collisionNormal2 = glm::normalize(glm::vec2(0.0f, 1.0f));
        // printf("ColliderManager - C4\n");

    }
    else
    {
        collisionNormal2 = glm::vec2(0.0f, 0.0f);
        printf("ColliderManager - C5\n");
    }

    collisionNormal1 = collisionNormal2 == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(-collisionNormal2);

    if(p_velocity_1 != nullptr)
    {
        float dotProduct1 = glm::dot(velocity1, collisionNormal2);

        if(dotProduct1 <= 0.0f)
        {
            glm::vec2 newVelocity1 = velocity1 - collisionNormal2 * dotProduct1;
        
            p_velocity_1->SetVelocity(newVelocity1);
        }
    }

    if(p_velocity_2 != nullptr)
    {
        float dotProduct2 = glm::dot(velocity2, collisionNormal1);
        
        if(dotProduct2 <= 0.0f)
        {
            glm::vec2 newVelocity2 = velocity2 - collisionNormal1 * dotProduct2;
    
            p_velocity_2->SetVelocity(newVelocity2);
        }
    }
}

// void ColliderManager::PushAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
// {
//     glm::vec2 velocity1 = p_velocity_1 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_1->GetVelocity();
//     glm::vec2 velocity2 = p_velocity_2 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_2->GetVelocity();
    
//     glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta;
    
//     float   left1, right1, top1, bottom1,
//             left2, right2, top2, bottom2;

//     left1 = p_translation_1.x;
//     right1 = p_translation_1.x + p_dimensions_1.x;
//     top1 = p_translation_1.y;
//     bottom1 = p_translation_1.y - p_dimensions_1.y;


//     left2 = p_translation_2.x;
//     right2 = p_translation_2.x + p_dimensions_2.x;
//     top2 = p_translation_2.y;
//     bottom2 = p_translation_2.y - p_dimensions_2.y;

//     glm::vec2 collisionNormal1 = glm::vec2(0.0f, 0.0f);
//     glm::vec2 collisionNormal2 = glm::vec2(0.0f, 0.0f);

//     glm::vec2 overlap = glm::vec2(
//         std::min(right1 - left2, right2 - left1),
//         std::min(top1 - bottom2, top2 - bottom1)
//     );

//     // obj1 pushes obj2
//     if(p_velocity_1 == nullptr && p_velocity_2 != nullptr)
//     {  
//         glm::vec2 shiftVector = glm::vec2(0.0f, 0.0f);
        
//         shiftVector.x = velocity2.x > 0.0f ? overlap.x : -overlap.x;
//         shiftVector.y = velocity2.y > 0.0f ? -overlap.y : overlap.y;
        
//         p_velocity_2->GetGameObject()->GetComponent<wolf::Transform2D>()->Translate(glm::vec2(shiftVector));
//     }   

//     // // obj2 pushes obj1
//     else if(p_velocity_1 != nullptr && p_velocity_2 == nullptr)
//     {
//         glm::vec2 shiftVector = glm::vec2(0.0f, 0.0f);
        
//         shiftVector.x = velocity1.x > 0.0f ? overlap.x : -overlap.x;
//         shiftVector.y = velocity1.y > 0.0f ? -overlap.y : overlap.y;
        
//         p_velocity_1->GetGameObject()->GetComponent<wolf::Transform2D>()->Translate(glm::vec2(shiftVector));
//     }
//     // obj1 & obj2 both push
//     else if(p_velocity_1 != nullptr && p_velocity_2 != nullptr)
//     {
//         bool isObj1FasterThanObj2 = glm::length(velocity1) > glm::length(velocity2);
//         glm::vec2 relativeVelocity = isObj1FasterThanObj2 ? velocity2 - velocity1 : velocity1 - velocity2;
//         glm::vec2 benchmarkVelocity = isObj1FasterThanObj2 ? velocity2 : velocity1; 
//         glm::vec2 shiftVector = glm::vec2(0.0f, 0.0f);
        
//         shiftVector.x = benchmarkVelocity.x > 0.0f ? overlap.x : -overlap.x;
//         shiftVector.y = benchmarkVelocity.y > 0.0f ? -overlap.y : overlap.y;
        
//         if(isObj1FasterThanObj2)
//         {
//             p_velocity_2->GetGameObject()->GetComponent<wolf::Transform2D>()->Translate(glm::vec2(shiftVector));

//         }
//         else
//         {
//             p_velocity_1->GetGameObject()->GetComponent<wolf::Transform2D>()->Translate(glm::vec2(shiftVector));

//         }
//     }
// }

// bool ColliderManager::StandardAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2)
// {
//     return !(
//         p_translation_1.x + p_dimensions_1.x < p_translation_2.x                        ||  // Right1 > Left2
//         p_translation_1.x                    > p_translation_2.x + p_dimensions_2.x     ||  // Left1 < Right2
//         p_translation_1.y - p_dimensions_1.y > p_translation_2.y                        ||  // Lower1 > Upper2
//         p_translation_1.y                    < p_translation_2.y - p_dimensions_2.y         // Upper1 < Lower2
//     );
// }

// bool ColliderManager::StandardAABBBroadphase(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
// {
//     glm::vec2 relativeVelocity = glm::vec2(0.0f);
//     relativeVelocity = p_velocity_1 == nullptr ? relativeVelocity : relativeVelocity + p_velocity_1->GetVelocity();
//     relativeVelocity = p_velocity_2 == nullptr ? relativeVelocity : relativeVelocity - p_velocity_2->GetVelocity();
//     relativeVelocity *= p_delta;

//     glm::vec2 broadphaseTranslation = p_translation_1 + relativeVelocity;

//     glm::vec2 broadphaseDimensions = p_dimensions_1;

//     bool result = !(
//         broadphaseTranslation.x + broadphaseDimensions.x    <   p_translation_2.x                         ||  // Right1 > Left2
//         broadphaseTranslation.x                             >   p_translation_2.x + p_dimensions_2.x      ||  // Left1 < Right2
//         broadphaseTranslation.y - broadphaseDimensions.y    >   p_translation_2.y                         ||  // Lower1 > Upper2
//         broadphaseTranslation.y                             <   p_translation_2.y - p_dimensions_2.y          // Upper1 < Lower2
//     );

//     return result;
// }

// float ColliderManager::SweptAABB(glm::vec2 p_translation_1, glm::vec2 p_translation_2, glm::vec2 p_dimensions_1, glm::vec2 p_dimensions_2, VelocityComponent* p_velocity_1, VelocityComponent* p_velocity_2, float p_delta)
// {
//     if(this->StandardAABBBroadphase(p_translation_1, p_translation_2, p_dimensions_1, p_dimensions_2, p_velocity_1, p_velocity_2, p_delta))
//     {
//         //std::cout << "Collider Manager - Broadphased - id1: " << p_velocity_1->GetGameObject()->GetID() << ", id2: " << p_velocity_2->GetGameObject()->GetID() << std::endl;
//         float   xEntryDist, yEntryDist, xExitDist, yExitDist,
//                 xEntryTime, yEntryTime, xExitTime, yExitTime,
//                 entryTime, exitTime;

//         // Distance calculations
//         int colCaseX, colCaseY = 0;

//         glm::vec2 velocity1 = p_velocity_1 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_1->GetVelocity();
//         glm::vec2 velocity2 = p_velocity_2 == nullptr ? glm::vec2(0.0f, 0.0f) : p_velocity_2->GetVelocity();

//         glm::vec2 relativeVelocity = (velocity1 - velocity2) * p_delta; // Velocity of obj1 as observed from obj2

//         if(relativeVelocity == glm::vec2(0.0f, 0.0f))
//         {
//             printf("ColliderManager - C0\n");
//             return 0.0f;
//         }

//         glm::vec2 shiftedTranslation1;

//         shiftedTranslation1 = p_translation_1;
//         shiftedTranslation1.x = relativeVelocity.x > 0.0f ? p_translation_2.x - p_dimensions_1.x: p_translation_2.x + p_dimensions_2.x;
//         shiftedTranslation1.y = relativeVelocity.y > 0.0f ? p_translation_2.y - p_dimensions_2.y: p_translation_2.y + p_dimensions_1.y;

//         if(relativeVelocity.x > 0.0f)
//         {
//             xEntryDist = p_translation_2.x - (shiftedTranslation1.x + p_dimensions_1.x);
//             xExitDist = (p_translation_2.x + p_dimensions_2.x) - shiftedTranslation1.x;
//             colCaseX = 1;
//         }
//         else
//         {
//             xEntryDist = (p_translation_2.x + p_dimensions_2.x) - shiftedTranslation1.x;
//             xExitDist = p_translation_2.x - (shiftedTranslation1.x + p_dimensions_1.x);
//             colCaseX = 0;
//         }

//         if (relativeVelocity.y > 0.0f)
//         {
//             yEntryDist = (p_translation_2.y - p_dimensions_2.y) - shiftedTranslation1.y;
//             yExitDist = p_translation_2.y - (shiftedTranslation1.y - p_dimensions_1.y);
//             colCaseY = 1;
//         }
//         else
//         {
//             yEntryDist = p_translation_2.y - (shiftedTranslation1.y - p_dimensions_1.y);
//             yExitDist = (p_translation_2.y - p_dimensions_2.y) - shiftedTranslation1.y;
//             colCaseY = 0;
//         }

//         // Time calculations
//         if(relativeVelocity.x == 0.0f)
//         {
//             xEntryTime = -std::numeric_limits<float>::infinity();
//             xExitTime = std::numeric_limits<float>::infinity();
//         }
//         else
//         {
//             xEntryTime = xEntryDist / relativeVelocity.x;
//             xExitTime = xExitDist / relativeVelocity.x;
//         }

//         if(relativeVelocity.y == 0.0f)
//         {
//             yEntryTime = -std::numeric_limits<float>::infinity();
//             yExitTime = std::numeric_limits<float>::infinity();
//         }
//         else
//         {
//             yEntryTime = yEntryDist / relativeVelocity.y;
//             yExitTime = yExitDist / relativeVelocity.y;
//         }
//         entryTime = std::max(xEntryTime, yEntryTime);
//         exitTime = std::min(xExitTime, yExitTime);

//         // Non-collision check
//         if
//         (
//             (entryTime > exitTime) ||
//             (xEntryTime < 0.0f && yEntryTime < 0.0f) ||
//             (xEntryTime > 1.0f) ||
//             (yEntryTime > 1.0f)
//         )
//         {
//             float   oldLeft1, oldRight1,oldTop1, oldBottom1,
//                     left1, right1, top1, bottom1,
//                     left2, right2, top2, bottom2;

//             oldLeft1 = p_translation_1.x;
//             oldRight1 = p_translation_1.x + p_dimensions_1.x;
//             oldTop1 = p_translation_1.y;
//             oldBottom1 = p_translation_1.y - p_dimensions_1.y;

//             left1 = oldLeft1 + relativeVelocity.x;
//             right1 = oldRight1 + relativeVelocity.x;
//             top1 = oldTop1 + relativeVelocity.y;
//             bottom1 = oldBottom1 + relativeVelocity.y;


//             left2 = p_translation_2.x;
//             right2 = p_translation_2.x + p_dimensions_2.x;
//             top2 = p_translation_2.y;
//             bottom2 = p_translation_2.y - p_dimensions_2.y;

//             // std::cout << "ColliderManager - id1: " << p_velocity_1->GetGameObject()->GetID() << ", id2: " <<  p_velocity_2->GetGameObject()->GetID() << std::endl;
//             // std::cout << "ColliderManager - entryTime: " << entryTime << std::endl;
//             // std::cout << "ColliderManager - exitTime: " << exitTime << std::endl;
//             // std::cout << "ColliderManager - xEntryTime: " << xEntryTime << std::endl;
//             // std::cout << "ColliderManager - yEntryTime: " << xEntryTime << std::endl;
//             // std::cout << "ColliderManager - oldTop1: " << oldTop1 << ", oldBottom1: " << oldBottom1 << ", oldLeft1: " << oldLeft1 << ", oldRight1: " << oldRight1 << std::endl;
//             // std::cout << "ColliderManager - top1: " << top1 << ", bottom1: " << bottom1 << ", left1: " << left1 << ", right: " << right1 << std::endl;
//             // std::cout << "ColliderManager - top2: " << top2 << ", bottom2: " << bottom2 << ", left2: " << left2 << ", right: " << right2 << std::endl;
            
//             return 100.0f;
//         }
//         else
//         {
            
//             this->SlideAABB(p_translation_1, p_translation_2, p_dimensions_1, p_dimensions_2, p_velocity_1, p_velocity_2, p_delta);
//             return entryTime;
//         }
//     }
//     return 100.0f;
// }