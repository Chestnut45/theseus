//-----------------------------------------------------------------------------
// File: HitboxComponent.h
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Hitbox.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>

class HitboxComponent : public wolf::BaseComponent
{
public:
    HitboxComponent();
    HitboxComponent(glm::vec2 p_dimensions);

    glm::vec2 GetDimensions() const;

private:
    glm::vec2 m_dimensions = glm::vec2(1.0f, 1.0f); // Default dimensions
};