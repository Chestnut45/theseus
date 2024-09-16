//-----------------------------------------------------------------------------
// File: HurtboxComponent.h
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Hurtbox.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class HurtboxComponent : public wolf::BaseComponent
{
public:  
HurtboxComponent() = default;
HurtboxComponent(glm::vec2 p_dimensions);

glm::vec2 getHitboxDimensions() const;

private:
    glm::vec2 m_dimensions = glm::vec2(1.0f, 1.0f); // Default dimensions
};