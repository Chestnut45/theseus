//-----------------------------------------------------------------------------
// File: ProjectileComponent.h
// Original Author: Nguyễn Minh Nhật
// .
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include "../ColliderManager.h"

class AttackDamageComponent : public wolf::BaseComponent
{
public:
    AttackDamageComponent(float p_damage, ColliderManager* p_collider_manager);
    virtual ~AttackDamageComponent();

    void Update(float p_dt);

private:
    float m_fDamage = 0.0f;
    ColliderManager * m_pColliderManager = nullptr;
};