#include "VelocityComponent.h"
#include "W_Transform2D.h"

//-----------------------------------------------------------------------------
// File:            VelocityComponent.cpp
// Original Author: Youssef Ashraf
// ver 1.2, updated to handle object transform.
//-----------------------------------------------------------------------------

#include "VelocityComponent.h"

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