#include "VelocityComponent.h"
#include "W_Transform2D.h"

//-----------------------------------------------------------------------------
// File:            VelocityComponent.cpp
// Original Author: Youssef Ashraf
// ver 1.3, updated to handle dynamic blending of knockback and input velocity.
//-----------------------------------------------------------------------------

void VelocityComponent::SetVelocity(const glm::vec2& velocity) {
    // Allow input velocity updates even during knockback, blending with knockback velocity
    if (m_knockbackActive) {
        BlendVelocity(velocity);
    } else {
        m_velocity = velocity;
    }
}

const glm::vec2& VelocityComponent::GetVelocity() const {
    return m_velocity;
}

void VelocityComponent::ApplyKnockback(const glm::vec2& direction, float force) {
    m_knockbackVelocity = direction * force;
    m_knockbackActive = true;
    m_knockbackTimeElapsed = 0.0f;
}

bool VelocityComponent::IsKnockbackActive() const {
    return m_knockbackActive;
}

void VelocityComponent::UpdateKnockback(float delta) {
    if (!m_knockbackActive) return;

    // Update knockback time and apply damping
    m_knockbackTimeElapsed += delta;
    m_knockbackVelocity *= m_dampingFactor;

    // Blend factor decreases over time, giving more control back to the input velocity
    float progress = m_knockbackTimeElapsed / m_knockbackDuration;
    m_blendFactor = glm::clamp(1.0f - progress, 0.0f, 1.0f);

    // Stop knockback when the duration is over or velocity is very small
    if (m_knockbackTimeElapsed >= m_knockbackDuration || glm::length(m_knockbackVelocity) < 0.1f) {
        m_knockbackVelocity = glm::vec2(0.0f);
        m_knockbackActive = false;
        m_blendFactor = 0.5f;  // Reset blend factor
    }
}

void VelocityComponent::BlendVelocity(const glm::vec2& inputVelocity) {
    // Blend the input velocity and knockback velocity based on the current blend factor
    m_velocity = (m_knockbackVelocity * m_blendFactor) + (inputVelocity * (1.0f - m_blendFactor));
}
