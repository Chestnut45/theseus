//-----------------------------------------------------------------------------
// File: HealthComponent.h
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------
#pragma once 

#include <glm/glm.hpp>
#include <wolf.h>

#include "ArmourComponent.h"

class HealthComponent : public wolf::BaseComponent
{
public:
    HealthComponent() = default;
    HealthComponent(int p_health);
    ~HealthComponent();
    
    void Init();
    float GetMaxHealth() const;

    float GetHealth() const;

    void Damage(float p_damage);
    void Heal(float p_heal);
    void Supercharge(float p_supercharge);

private:
    float m_health = 100;
    float m_cap = 100;
};