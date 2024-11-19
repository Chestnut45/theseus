//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include "../ColliderManager.h"
#include "StatusComponent.h"


class AttackDamageComponent : public wolf::BaseComponent
{
public:
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager);

    // Constructor with optional arguments for applying knockback / status effects
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, float knockbackMagnitude = 0.0f, std::vector<std::pair<StatusComponent::StatusEffectType, float>> p_status_effects = {});
    virtual ~AttackDamageComponent();

    void Update(float p_dt);

    std::pair<StatusComponent::StatusEffectType, float> defaultVal = std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::NONE, 0);


private:
    float m_fDamage = 0.0f;
    ColliderManager * m_pColliderManager = nullptr;
    float m_knockbackMagnitude = 0.0f; // Knockback magnitude (default is no knockback)
    float m_aStatusEffectsLifespans[StatusComponent::StatusEffectType::NONE];
};