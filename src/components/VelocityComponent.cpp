//-----------------------------------------------------------------------------
// File:			VelocityComponent.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Velocity component to be added in certain game objects.
//-----------------------------------------------------------------------------
#include "VelocityComponent.h"

VelocityComponent::VelocityComponent(wolf::Transform2D* pTransform)
    : m_pTransform(pTransform), m_velocity(0.0f, 0.0f)
{
}

void VelocityComponent::Update(float delta)
{
    // Apply velocity to the transform, updating the position
    glm::vec2 newPosition = m_pTransform->GetLocalPosition() + (m_velocity * delta);
    m_pTransform->SetPosition(newPosition);
}

// set the velocity vector
void VelocityComponent::SetVelocity(const glm::vec2& velocity)
{
    m_velocity = velocity;
}

// get the current velocity vector
const glm::vec2& VelocityComponent::GetVelocity() const
{
    return m_velocity;
}