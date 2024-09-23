//-----------------------------------------------------------------------------
// File: HitboxComponent.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Hitbox.
//-----------------------------------------------------------------------------

#include "HitboxComponent.h"

HitboxComponent::HitboxComponent()
{
}

// Constructor for custom dimensions
HitboxComponent::HitboxComponent(glm::vec2 p_dimensions)
{
    this->m_dimensions = p_dimensions;
    
}

// Get dimensions
glm::vec2 HitboxComponent::GetDimensions() const
{
    return this->m_dimensions;
}