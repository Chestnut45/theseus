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
HurtboxComponent(glm::vec2 p_dimensions, bool p_type);

glm::vec2 GetDimensions() const;
bool GetType() const;

private:
    glm::vec2 m_dimensions = glm::vec2(1.0f, 1.0f); // Default dimensions
    bool m_type = 0; // Type - 0: Damage Receiver, 1: Damage Dealer
};