//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------
#pragma once

#include <wolf.h>
#include "../ColliderManager.h"
#include "../inventory/WeaponItem.h"
#include "AttackSourceType.h"

class AttackDamageComponent : public wolf::BaseComponent {
public:
    // Constructor for weapon-based attack
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, WeaponType p_weaponType);

    // Constructor for non-weapon-based attack (e.g., Harpy, ThrowableObject)
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, AttackSourceType p_sourceType);

    virtual ~AttackDamageComponent();

    void Update(float p_dt);

private:
    float m_fDamage = 0.0f;
    ColliderManager* m_pColliderManager = nullptr;
    WeaponType m_weaponType;
    AttackSourceType m_sourceType;
    bool m_isWeaponAttack = false;  // Flag to indicate if the attack is weapon-based

    // Helper method to apply knockback based on weapon or source type
    void ApplyKnockback(wolf::GameObject* target, const glm::vec2& direction);
};
