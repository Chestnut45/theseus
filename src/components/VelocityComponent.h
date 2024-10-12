#pragma once

//-----------------------------------------------------------------------------
// File:            VelocityComponent.h
// Original Author: Youssef Ashraf
// ver 1.1.
// A class that's responsible for the Velocity component to be added in certain game objects.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class VelocityComponent : public wolf::BaseComponent
{
public:
    VelocityComponent() = default;  // No arguments needed

    void SetVelocity(const glm::vec2& velocity);  // Setter for velocity
    const glm::vec2& GetVelocity() const;  // Getter for velocity
    const glm::vec2 GetNormalisedVelocity() const; // Getter for normalised velocity

    void Knockback(float p_knockback_force, glm::vec2 p_knockback_direction);

private:
    float m_fKnockbackForce = 0.0f;
    glm::vec2 m_fKnockbackDirection = glm::vec2(1.0f);
    glm::vec2 m_velocity = glm::vec2(0.0f, 0.0f);  // Store velocity
};