#pragma once

//-----------------------------------------------------------------------------
// File:            VelocityComponent.h
// Original Author: Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
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
    void ApplyKnockback(const glm::vec2& direction, float magnitude);

    void Update(float deltaTime); // To update velocity over time

    void SetActive(bool active) { m_active = active; }
    bool IsActive() const {return m_active; }

    void SetKnockbackEnabled(bool value) { m_knockbackEnabled = value; }
    bool IsKnockbackEnabled() const { return m_knockbackEnabled; }

private:
    bool m_active = true;
    bool m_knockbackEnabled = true;
    glm::vec2 m_velocity = glm::vec2(0.0f, 0.0f);  // Store velocity
    float m_friction = 10.0f; // Friction coefficient to slow down velocity
    glm::vec2 m_knockbackStartVelocity = glm::vec2(0.0f, 0.0f); // Initial velocity when knockback starts
    float m_knockbackRecoveryTime = 0.0f; // Time for the knockback to fade out
};