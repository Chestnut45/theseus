//-----------------------------------------------------------------------------
// File: HealthComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------

#include "HealthComponent.h"

// Constructor for custom health
HealthComponent::HealthComponent(int p_health)
{
    this->m_health = p_health;
    this->m_cap = p_health;

    // Add Listeners for the healing events related to items
    wolf::EventManager::AddListener<PercentHealthItemEvent, HealthComponent, &HealthComponent::HandlePercentHealthItemEvent>(*this);
    wolf::EventManager::AddListener<FlatHealthItemEvent, HealthComponent, &HealthComponent::HandleFlatHealthItemEvent>(*this);
}

// Destructor
HealthComponent::~HealthComponent()
{
    // Remove the healing item event Listeners
    wolf::EventManager::RemoveListener<PercentHealthItemEvent, HealthComponent, &HealthComponent::HandlePercentHealthItemEvent>(*this);
    wolf::EventManager::RemoveListener<FlatHealthItemEvent, HealthComponent, &HealthComponent::HandleFlatHealthItemEvent>(*this);
}

void HealthComponent::Init()
{
}

// Get health
float HealthComponent::GetHealth() const
{
    return this->m_health;
}

// Reduce health
void HealthComponent::Damage(float p_damage)
{
    if(this->m_health > 0)
    {
        
        this->m_health -= p_damage;

        if (m_health < 0) m_health = 0;

        // std::cout << "HealthComponent - Health: " << this->m_health << std::endl;

        if(this->m_health <= 0)
        {
            //---------------------------------------//
            //                                       //
            // SEND EVENT TO INDICATE ENTITY IS DEAD //
            //                                       //
            //---------------------------------------//
        }
    }
}

// Increase health (with cap)
void HealthComponent::Heal(float p_heal)
{
    this->m_health += p_heal;
    if(this->m_health > this->m_cap)
    {
        this->m_health = this->m_cap;
    }
}

// Increase cap & refill health
void HealthComponent::Supercharge(float p_supercharge)
{
    this->m_cap += p_supercharge;
    this->m_health = this->m_cap;
}

float HealthComponent::GetMaxHealth() const
{
    return m_cap;  
}

// !-- Aurora added these events --!
void HealthComponent::HandlePercentHealthItemEvent(const PercentHealthItemEvent& p_event) {
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt * m_cap);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt * m_cap);
    }
}

void HealthComponent::HandleFlatHealthItemEvent(const FlatHealthItemEvent& p_event) {
    if (p_event.fHealthChangeAmt >= 0) {
        this->Heal(p_event.fHealthChangeAmt);
    }
    else {
        this->Damage(-p_event.fHealthChangeAmt);
    }
}