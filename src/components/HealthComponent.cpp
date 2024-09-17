//-----------------------------------------------------------------------------
// File: HealthComponent.cpp
// Original Author: Nguyễn Minh Nhật
// ver 1.1.
// Health.
//-----------------------------------------------------------------------------

#include "HealthComponent.h"

// Constructor for custom health
HealthComponent::HealthComponent(int p_health)
{
    this->m_health = p_health;
    this->m_cap = p_health;
}

// Destructor
HealthComponent::~HealthComponent()
{
}

void HealthComponent::Init()
{
}

// Get health
int HealthComponent::GetHealth() const
{
    return this->m_health;
}

// Reduce health
void HealthComponent::Damage(int p_damage)
{
    ArmourComponent* armourComponent = this->GetGameObject()->GetComponent<ArmourComponent>();
    if(p_damage > 0)
    {
        if(armourComponent != nullptr)
        {
            this->m_health -= std::round(p_damage * (100 - armourComponent->GetMultiplier()) * 0.01f);
        }
        else
        {
            this->m_health -= p_damage;
        }  
    }
}

// Increase health (with cap)
void HealthComponent::Heal(int p_heal)
{
    this->m_health += p_heal;
    if(this->m_health > this->m_cap)
    {
        this->m_health = this->m_cap;
    }
}

// Increase health (without cap)
void HealthComponent::Supercharge(int p_supercharge)
{
    this->m_health = p_supercharge;
    this->m_cap = p_supercharge;
}