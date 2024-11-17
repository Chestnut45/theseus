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

    // Delete copy constructor/assignment
    VelocityComponent(const VelocityComponent&) = delete;
    VelocityComponent& operator=(const VelocityComponent&) = delete;

    // Delete move constructor/assignment
    VelocityComponent(VelocityComponent&& other) = delete;
    VelocityComponent& operator=(VelocityComponent&& other) = delete;

    void SetVelocity(const glm::vec2& velocity);  // Setter for velocity
    const glm::vec2& GetVelocity() const;  // Getter for velocity

    // Knockback methods
    void ApplyKnockback(const glm::vec2& direction, float force);
    bool IsKnockbackActive() const;
    void UpdateKnockback(float delta);
    void BlendVelocity(const glm::vec2& inputVelocity);



private:
    glm::vec2 m_velocity = glm::vec2(0.0f, 0.0f);
    glm::vec2 m_knockbackVelocity = glm::vec2(0.0f);
    bool m_knockbackActive = false;
    float m_dampingFactor = 0.90f;
    float m_knockbackDuration = 0.5f;  // Total time for the knockback effect
    float m_knockbackTimeElapsed = 0.0f;
    float m_blendFactor = 0.5f;  // Controls the blend between knockback and input velocity

};