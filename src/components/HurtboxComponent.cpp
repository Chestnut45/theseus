//-----------------------------------------------------------------------------
// File: HurtboxComponent.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Hurtbox.
//-----------------------------------------------------------------------------

#include "HurtboxComponent.h"

// Custom constructors for dimensiosn
HurtboxComponent::HurtboxComponent(glm::vec2 p_dimensions)
{
    this->m_dimensions = p_dimensions;

}

// Get dimensions
glm::vec2 HurtboxComponent::getHitboxDimensions() const
{
    return this->m_dimensions;
}