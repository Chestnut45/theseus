#include "VelocityComponent.h"
#include "W_Transform2D.h"

//-----------------------------------------------------------------------------
// File:            VelocityComponent.cpp
// Original Author: Youssef Ashraf
// ver 1.2, updated to handle object transform.
//-----------------------------------------------------------------------------

// Constructor (if needed, but not required since it's default)
// The default constructor does nothing special for now, but can be expanded if needed

// Set velocity method
void VelocityComponent::SetVelocity(const glm::vec2& velocity)
{
    m_velocity = velocity;
}

// Get velocity method
const glm::vec2& VelocityComponent::GetVelocity() const
{
    return m_velocity;
}

void VelocityComponent::ApplyKnockback(const glm::vec2& direction, float magnitude)
{
    // Ensure the direction is normalized
    glm::vec2 normalizedDirection = glm::dot(direction, direction) > 0.0f ? glm::normalize(direction) : glm::vec2(0.0f);

    // Apply knockback
    m_velocity += normalizedDirection * magnitude;
}

void VelocityComponent::Update(float deltaTime)
{
    // Apply friction to gradually bring velocity to zero
    if (glm::length(m_velocity) > 0.0f)
    {
        glm::vec2 frictionForce = -glm::normalize(m_velocity) * m_friction * deltaTime;
        
        // Ensure we don't overshoot zero velocity
        if (glm::length(frictionForce) > glm::length(m_velocity))
        {
            m_velocity = glm::vec2(0.0f);
        }
        else
        {
            m_velocity += frictionForce;
        }
    }
}