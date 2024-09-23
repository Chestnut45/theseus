//-----------------------------------------------------------------------------
// File: HurtboxComponent.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Hurtbox.
//-----------------------------------------------------------------------------

#include "HurtboxComponent.h"

// Custom constructors for dimensiosn
HurtboxComponent::HurtboxComponent(glm::vec2 p_dimensions, bool p_type)
{
    this->m_dimensions = p_dimensions;
    this->m_type = p_type;

}

// Get dimensions
glm::vec2 HurtboxComponent::GetDimensions() const
{
    return this->m_dimensions;
}

// Get type
bool HurtboxComponent::GetType() const
{
    return this->m_type;
}