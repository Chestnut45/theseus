//-----------------------------------------------------------------------------
// File: StatusComponent.h
// Original Author: Nguyễn Minh Nhật
// Status.
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "ArmourComponent.h"
#include "HealthComponent.h"

class StatusComponent : public wolf::BaseComponent
{
public:
    enum StatusEffectType
    {
        BURNING,
        PETRIFIED,
        POISONED,
        NONE
    };

    StatusComponent();
    virtual ~StatusComponent();

    void ApplyStatusEffect();

private:
    float m_fLifespan = 0.0f;
    
    struct StatusEffect
    {
        StatusEffectType m_StatusEffectType;
        wolf::Timer * m_pTimer = nullptr;

        StatusEffect()
        {
            
        }
    };
};