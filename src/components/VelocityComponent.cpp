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

    // Compute the knockback velocity
    glm::vec2 knockbackVelocity = normalizedDirection * magnitude;

    // Combine knockback with current velocity
    m_knockbackStartVelocity = m_velocity + knockbackVelocity; // Add the knockback to the current velocity
    m_velocity = m_knockbackStartVelocity;

    // Set recovery parameters
    m_knockbackRecoveryTime = 1.0f; // Recovery duration in seconds
}


void VelocityComponent::Update(float deltaTime)
{
    // Handle knockback fade-out effect
    if (m_knockbackRecoveryTime > 0.0f)
    {
        // Use damping to simulate recovery
        float dampingFactor = glm::exp(-5.0f * (1.0f - m_knockbackRecoveryTime)); // Adjust exponential factor if too much/too little
        glm::vec2 knockbackEffect = m_knockbackStartVelocity * dampingFactor;

        // Blend knockback with current velocity (set externally)
        m_velocity = glm::mix(knockbackEffect, m_velocity, 0.5f); // Adjust blending factor for responsiveness
        
        m_knockbackRecoveryTime -= deltaTime;

        // Reduce recovery time
        m_knockbackRecoveryTime -= deltaTime;
        if (m_knockbackRecoveryTime <= 0.0f)
        {
            m_velocity = glm::vec2(0.0f); // Stop movement completely
            m_knockbackRecoveryTime = 0.0f;
        }
    }
    else
    {
        // Regular friction-based deceleration
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
}