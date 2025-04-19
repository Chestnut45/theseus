#pragma once

//-----------------------------------------------------------------------------
// File: AttackDamageComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Modifications: Youssef Ashraf
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------
#include <wolf.h>
#include <ColliderManager.h>
#include <StatusComponent.h>


class AttackDamageComponent : public wolf::BaseComponent
{
public:

    // Constructor with optional arguments for applying knockback / status effects
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, float knockbackMagnitude = 0.0f, std::vector<std::pair<StatusComponent::StatusEffectType, float>> p_status_effects = {}, wolf::GameObject* owner = nullptr);
    virtual ~AttackDamageComponent();

    void Update(float p_dt);

    std::pair<StatusComponent::StatusEffectType, float> defaultVal = std::pair<StatusComponent::StatusEffectType, float>(StatusComponent::StatusEffectType::NONE, 0);

    wolf::GameObject* GetOwner() const { return m_pOwner; } // Getter for owner
private:
    float m_fDamage = 0.0f;
    ColliderManager * m_pColliderManager = nullptr;
    float m_knockbackMagnitude = 0.0f; // Knockback magnitude (default is no knockback)
    float m_aStatusEffectsLifespans[StatusComponent::StatusEffectType::NONE];
    wolf::GameObject* m_pOwner = nullptr; //  Owner reference (optional)

};