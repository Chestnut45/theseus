#include "VelocityComponent.h"
#include "W_Transform2D.h"

//-----------------------------------------------------------------------------
// File:            VelocityComponent.cpp
// Original Author: Youssef Ashraf
// ver 1.2, updated to handle object transform.
//-----------------------------------------------------------------------------

#include "VelocityComponent.h"

int VelocityComponent::s_iComponentCount = 0;

// Constructor

VelocityComponent::VelocityComponent()
{
    VelocityComponent::s_iComponentCount++;
}

VelocityComponent::~VelocityComponent()
{
    VelocityComponent::s_iComponentCount--;
}

// Set velocity method
void VelocityComponent::SetVelocity(const glm::vec2& velocity)
{
    m_velocity = velocity;
}

// Get velocity method
glm::vec2 VelocityComponent::GetVelocity() const
{
    glm::vec2 res = this->m_velocity + (this->m_fKnockbackDirection * this->m_fKnockbackForce * 100.0f);
    return res;
}

glm::vec2 VelocityComponent::GetNormalisedVelocity() const
{

    if(this->m_velocity == glm::vec2(0.0f, 0.0f))
    {
        return glm::vec2(0.0f, 0.0f);
    }
    return glm::normalize(this->m_velocity);
}

void VelocityComponent::Knockback(float p_knockback_force, glm::vec2 p_knockback_direction)
{
    this->m_fKnockbackForce = p_knockback_force;
    this->m_fKnockbackDirection = p_knockback_direction;
    //std::cout << "VelocityComponent - kb_direction - id: " << this->GetGameObject()->GetID() << " - x: " << p_knockback_direction.x << ", y: " << p_knockback_direction.y << std::endl;

}