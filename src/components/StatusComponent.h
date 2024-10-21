//-----------------------------------------------------------------------------
// File: StatusComponent.h
// Original Author: Nguyễn Minh Nhật
// Status.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------
#pragma once

#include <glm/glm.hpp>
#include <wolf.h>

#include "ArmourComponent.h"
#include "HealthComponent.h"
#include "VelocityComponent.h"

#include "../ColliderManager.h"

class VelocityComponent;

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

    void AddStatusEffect(StatusEffectType p_se_type, float p_lifespan);
    bool IsStatusEffectActive(StatusEffectType p_se_type) const;

    void Update();

private:
    struct StatusEffect
    {
        void ApplyStatusEffect();

        bool m_isActive = false;
        float m_fLifespan = 1.0f;
        StatusEffectType m_StatusEffectType;
        wolf::Timer m_timer;
        StatusComponent* m_OwnerComponent = nullptr;
    };

    StatusEffect m_aStatusEffects [StatusEffectType::NONE];

    void RemoveStatusEffect(StatusEffectType p_se_type);
};