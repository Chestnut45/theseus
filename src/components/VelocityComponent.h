#pragma once
//-----------------------------------------------------------------------------
// File:			VelocityComponent.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Velocity component to be added in certain game objects.
//-----------------------------------------------------------------------------
#include <wolf.h>
#include <glm/glm.hpp>

class VelocityComponent : public wolf::BaseComponent
{
// forgot to make this public, was private by default
public:
    VelocityComponent() = default;
    VelocityComponent(wolf::Transform2D* pTransform);

    void Update(float delta);

    void SetVelocity(const glm::vec2& velocity);

    // Get the current velocity
    const glm::vec2& GetVelocity() const;

private:
    // Reference to the object's transform
    wolf::Transform2D* m_pTransform = nullptr;

    // Velocity vector (x, y)
    glm::vec2 m_velocity = glm::vec2(0.0f, 0.0f);
};