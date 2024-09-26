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

#include "../ColliderManager.h"

class StatusManager;

class StatusComponent : public wolf::BaseComponent
{
    friend StatusManager;

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

private:
    struct StatusEffect
    {
    public:
        StatusEffect(StatusEffectType p_se_type, float p_lifespan)
        {
            m_StatusEffectType = p_se_type;
            this->m_fLifespan = p_lifespan;
            this->m_pTimer = new wolf::Timer();
            this->m_pTimer->Start();
        }

        virtual ~StatusEffect()
        {
            delete this->m_pTimer;
            this->m_pTimer = nullptr;
        }

        float GetLifespan() const
        {
            return this->m_fLifespan;
        }

        StatusEffectType GetStatusEffectType() const
        {
            return this->m_StatusEffectType;
        }

        wolf::Timer* GetTimer() const
        {
            return this->m_pTimer;
        }

        void ApplyStatusEffect()
        {
            std::cout << "StatusComponent - Apply status effect: " << this->m_StatusEffectType << std::endl;
        }    

    private:
        float m_fLifespan = 0.0f;
        StatusEffectType m_StatusEffectType;
        wolf::Timer * m_pTimer = nullptr;
    };

    StatusEffect* m_aStatusEffects [StatusEffectType::NONE];
    bool m_aStatusEffectsPresenceFlags[StatusEffectType::NONE];

    void RemoveStatusEffect(StatusEffectType p_se_type);
};