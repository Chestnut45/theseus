//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// Checks for collision with HURTBOXDRs if owner object has HURTBOXDD.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include "../ColliderManager.h"

class AttackDamageComponent : public wolf::BaseComponent
{
public:
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager);

    //overloaded constructor for applying knockback
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager, float knockbackMagnitude);
    virtual ~AttackDamageComponent();

    void Update(float p_dt);

private:
    float m_fDamage = 0.0f;
    ColliderManager * m_pColliderManager = nullptr;
    float m_knockbackMagnitude = 0.0f; // Knockback magnitude (default is no knockback)
};