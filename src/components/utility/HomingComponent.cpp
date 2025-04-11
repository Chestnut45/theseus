//-----------------------------------------------------------------------------
// File: HomingComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Sets VelocityComponent towards target
//-----------------------------------------------------------------------------
#include "HomingComponent.h"

#include "VelocityComponent.h"

HomingComponent::HomingComponent(wolf::GameObject * p_target, float p_turning_cap, float p_delay, float p_active)
{
    this->m_pTarget = p_target;
    this->m_fTurningCapRad = p_turning_cap * (MATH_PI / 180.0f);
    this->m_iUpdateDelay = p_delay;
    this->m_iUpdateDelayCounter = 0.0f;
    this->m_bIsActive = p_active;
}

HomingComponent::~HomingComponent()
{
    this->m_pTarget = nullptr;
}

void HomingComponent::Update(float p_delta)
{
    if(m_bIsActive)
    {
        if(this->m_iUpdateDelayCounter >= this->m_iUpdateDelay)
        {
            this->m_iUpdateDelayCounter = 0.0f;
            VelocityComponent* ownerVelocityComponent = this->GetGameObject()->GetComponent<VelocityComponent>();
        
            if(ownerVelocityComponent != nullptr && ownerVelocityComponent->GetVelocity() != glm::vec2(0.0f, 0.0f))
            {
                wolf::Transform2D* ownerTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
                
                VelocityComponent* targetVelocityComponent = this->m_pTarget->GetComponent<VelocityComponent>();
                wolf::Transform2D* targetTransform = this->m_pTarget->GetComponent<wolf::Transform2D>();
                
                glm::vec2 ownerVelocity = ownerVelocityComponent->GetVelocity();
                glm::vec2 ownerTranslation = ownerTransform->GetGlobalPosition();
                glm::vec2 targetVelocity = targetVelocityComponent->GetVelocity();
                glm::vec2 targetTranslation = targetTransform->GetGlobalPosition();   
                
                glm::vec2 newTargetTranslation = targetTranslation + targetVelocity * p_delta;
                glm::vec2 toNewTarget = newTargetTranslation - ownerTranslation;
                toNewTarget = toNewTarget == glm::vec2(0.0f, 0.0f) ? glm::vec2(0.0f, 0.0f) : glm::normalize(toNewTarget);

                float dot = glm::dot(ownerVelocity, toNewTarget);
                float newLength = glm::length(toNewTarget);
                float ownerLength = glm::length(ownerVelocity);

                if(ownerLength > 0.0f && newLength > 0.0f)
                {
                    float cosine = std::clamp(dot / (ownerLength * newLength), -1.0f, 1.0f);
                    float radAngle = std::acos(cosine);

                    // If turning angle is smaller than cap
                    if(std::abs(radAngle) <= std::abs(this->m_fTurningCapRad))
                    {
                        ownerVelocityComponent->SetVelocity(toNewTarget * ownerLength);
                    }
                    else
                    {
                        float side = glm::cross(glm::vec3(ownerVelocity.x, ownerVelocity.y, 0), glm::vec3(toNewTarget.x, toNewTarget.y, 0)).z;
                        glm::vec2 toNewPos = glm::vec2(0.0f, 0.0f);
                        // Left
                        if(side >= 0.0f)
                        {
                            toNewPos.x = ownerVelocity.x * glm::cos(this->m_fTurningCapRad) - ownerVelocity.y * glm::sin(this->m_fTurningCapRad);
                            toNewPos.y = ownerVelocity.x * glm::sin(this->m_fTurningCapRad) + ownerVelocity.y * glm::cos(this->m_fTurningCapRad);
                            ownerVelocityComponent->SetVelocity(toNewPos);
                        }
                        // Right
                        else
                        {
                            toNewPos.x = ownerVelocity.x * glm::cos(-this->m_fTurningCapRad) - ownerVelocity.y * glm::sin(-this->m_fTurningCapRad);
                            toNewPos.y = ownerVelocity.x * glm::sin(-this->m_fTurningCapRad) + ownerVelocity.y * glm::cos(-this->m_fTurningCapRad);
                            ownerVelocityComponent->SetVelocity(toNewPos);
                        }
                    }
                }
            }
        }
        else
        {
            this->m_iUpdateDelayCounter += p_delta;
        }
    }
}

void HomingComponent::SetActive(bool p_active)
{
    m_bIsActive = p_active;
}

bool HomingComponent::IsActive()
{
    return m_bIsActive;
}