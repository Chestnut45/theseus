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
    void InflictStatusEffects();
    bool IsStatusEffectActive(StatusEffectType p_se_type) const;

private:
    struct StatusEffect
    {
    public:
        StatusEffect(StatusEffectType p_se_type, float p_lifespan, StatusComponent* p_owner_component)
        {
            m_StatusEffectType = p_se_type;
            this->m_fLifespan = p_lifespan;
            this->m_OwnerComponent = p_owner_component;
            this->m_pTimer = new wolf::Timer();
            this->m_pTimer->Start();
        }

        virtual ~StatusEffect()
        {
            if(this->m_StatusEffectType == StatusComponent::StatusEffectType::PETRIFIED)
            {
                VelocityComponent* velocityComponent = this->GetOwnerComponent()->GetGameObject()->GetComponent<VelocityComponent>();
                if(velocityComponent != nullptr)
                {
                    velocityComponent->SetPetrification(false);
                }
            }
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

        StatusComponent* GetOwnerComponent() const
        {
            return this->m_OwnerComponent;
        }

        void ApplyStatusEffect()
        {
            switch (this->m_StatusEffectType)
            {
                case StatusEffectType::BURNING:
                {
                    float damage = 0.1f;
                    HealthComponent* health = this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>();
                    if(health != nullptr)
                    {
                        ArmourComponent* armour = this->m_OwnerComponent->GetGameObject()->GetComponent<ArmourComponent>();
                        if(armour != nullptr && armour->IsSpecialPropertyPresent(ArmourComponent::SpecialProperty::FIRERESISTANCE));
                        {
                            damage *= (100 - armour->GetSpecialPropertiesValues(ArmourComponent::SpecialProperty::FIRERESISTANCE)) * 0.01f;
                        }
                        health->Damage(damage);
                    }
                    else
                    {
                        std::cout << "StatusComponent - ERROR: HealthComponent not found." << std::endl;
                    }
                    break;
                }

                case StatusEffectType::PETRIFIED:
                {
                    VelocityComponent* velocityComponent = this->GetOwnerComponent()->GetGameObject()->GetComponent<VelocityComponent>();
                    if(velocityComponent != nullptr)
                    {
                        velocityComponent->SetPetrification(true);
                    }
                    break;
                }      
                
                case StatusEffectType::POISONED:
                {
                    this->m_OwnerComponent->GetGameObject()->GetComponent<HealthComponent>()->Damage(0.2f);
                    break;
                }
            }
        }    

    private:
        float m_fLifespan = 0.0f;
        StatusEffectType m_StatusEffectType;
        wolf::Timer * m_pTimer = nullptr;
        StatusComponent* m_OwnerComponent;
    };

    StatusEffect* m_aStatusEffects [StatusEffectType::NONE];

    void RemoveStatusEffect(StatusEffectType p_se_type);
};