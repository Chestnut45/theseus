//-----------------------------------------------------------------------------
// File: StatusEffectComponent.h
// Original Author: Nguyễn Minh Nhật
// Base for status effects.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

class StatusEffectComponent : public wolf::BaseComponent
{
public:
    StatusEffectComponent() = default;

    void ApplyStatusEffect();

private:

};