//-----------------------------------------------------------------------------
// File: StatusComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Status.
// Notes: 
//      + DO NOT move StatusEffectType::NONE into any position other than last place in the enum
//-----------------------------------------------------------------------------

#include "StatusComponent.h"

#include "AnimatedSprite2D.h"
#include "VelocityComponent.h"

StatusComponent::StatusComponent()
{
    for(int i = 0; i < StatusEffectType::NONE; i++)
    {
        this->m_aStatusEffects[i].m_OwnerComponent = this;
        this->m_aStatusEffects[i].m_StatusEffectType = (StatusEffectType)i;
    }

    wolf::EventManager::AddListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

StatusComponent::~StatusComponent()
{
    wolf::EventManager::RemoveListener<ApplyStatusEffectEvent, StatusComponent, &StatusComponent::HandleApplyStatusEffectEvent>(*this);
}

// If status effect already present, reset timer
// else, add status effect
void StatusComponent::AddStatusEffect(StatusEffectType p_se_type, float p_lifespan)
{
    this->m_aStatusEffects[p_se_type].m_StatusEffectType = p_se_type;
    this->m_aStatusEffects[p_se_type].m_isActive = true;
    this->m_aStatusEffects[p_se_type].m_timer.Restart();
    this->m_aStatusEffects[p_se_type].m_fLifespan = p_lifespan;
}

bool StatusComponent::IsStatusEffectActive(StatusEffectType p_se_type) const
{
    return this->m_aStatusEffects[p_se_type].m_isActive;
}

void StatusComponent::Update()
{
    for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
    {
        StatusComponent::StatusEffect& statusEffect = this->m_aStatusEffects[i];
        
        if(statusEffect.m_isActive)
        {
            statusEffect.ApplyStatusEffect();

            if(statusEffect.m_fLifespan >= 0 && statusEffect.m_timer.Elapsed() >= statusEffect.m_fLifespan)
            {
                std::cout << "StatusComponent - Delete status effect: " << statusEffect.m_StatusEffectType << std::endl;
                
                this->RemoveStatusEffect(statusEffect.m_StatusEffectType);
            }
        }
    }
}

void StatusComponent::RemoveStatusEffect(StatusEffectType p_se_type)
{
    this->m_aStatusEffects[p_se_type].m_isActive = false;

    if(p_se_type == StatusEffectType::PETRIFIED)
    {
    AnimatedSprite2D* animatedSprite2DComponent = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
        if(animatedSprite2DComponent != nullptr)
        {
            animatedSprite2DComponent->SetTint(glm::vec3(1.0f));
        }
    }
}

void StatusComponent::StatusEffect::ApplyStatusEffect()
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
            VelocityComponent* velocityComponent = this->m_OwnerComponent->GetGameObject()->GetComponent<VelocityComponent>();
            if(velocityComponent != nullptr)
            {
                velocityComponent->SetVelocity(glm::vec2(0.0f, 0.0f));
            }

            AnimatedSprite2D* animatedSprite2DComponent = this->m_OwnerComponent->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if(animatedSprite2DComponent != nullptr)
            {
                animatedSprite2DComponent->SetTint(glm::vec3(1.5f, 1.5f, 1.5f));
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

// !-- Aurora added this method to be used with StatusEffectItems -- !
void StatusComponent::HandleApplyStatusEffectEvent(const ApplyStatusEffectEvent& p_event) {
    this->AddStatusEffect(static_cast<StatusComponent::StatusEffectType>(p_event.iType), p_event.fDuration);
}