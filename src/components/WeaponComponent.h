//-----------------------------------------------------------------------------
// File: WeaponComponent.h
// Original Author: Nguyễn Minh Nhật
// Base for weapons.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class WeaponComponent : public wolf::BaseComponent
{
public:
    WeaponComponent() = default;

    void Attack();

private:
    float m_fDelay = 0.0f;

};