//-----------------------------------------------------------------------------
// File: ArmourComponent.h
// Original Author: Nguyễn Minh Nhật
// Armour.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class ArmourComponent: public wolf::BaseComponent
{
public:
    ArmourComponent() = default;
    ArmourComponent(int p_multiplier);
    int GetMultiplier() const;
    void SetMultiplier(int p_multiplier);

private:
    int m_multiplier = 0; // Default multiplier

};