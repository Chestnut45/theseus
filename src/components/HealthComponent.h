//-----------------------------------------------------------------------------
// File: HealthComponent.h
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
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

    int GetHealth() const;

    void Damage(int p_damage);
    void Heal(int p_heal);
    void Supercharge(int p_health);

private:
    int m_health = 100;
    int m_cap = 100;
};