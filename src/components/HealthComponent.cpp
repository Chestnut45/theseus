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
    wolf::EventManager::AddListener<PercentHealItemEvent, HealthComponent, &HealthComponent::HandlePercentHealItemEvent>(*this);
    wolf::EventManager::AddListener<FlatHealItemEvent, HealthComponent, &HealthComponent::HandleFlatHealItemEvent>(*this);
}

// Destructor
HealthComponent::~HealthComponent()
{
    // Remove the healing item event Listeners
    wolf::EventManager::RemoveListener<PercentHealItemEvent, HealthComponent, &HealthComponent::HandlePercentHealItemEvent>(*this);
    wolf::EventManager::RemoveListener<FlatHealItemEvent, HealthComponent, &HealthComponent::HandleFlatHealItemEvent>(*this);
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
        ArmourComponent* armourComponent = this->GetGameObject()->GetComponent<ArmourComponent>();
        if(armourComponent != nullptr)
        {
            // std::cout << "HealthComponent - Damage: " << p_damage * ((100 - armourComponent->GetMultiplier()) * 0.01f) << std::endl;
            this->m_health -= p_damage * ((100 - armourComponent->GetMultiplier()) * 0.01f);           
        }
        else
        {
            // std::cout << "HealthComponent - Damage: " << p_damage << std::endl;
            this->m_health -= p_damage;
        }

        std::cout << "HealthComponent - Health: " << this->m_health << std::endl;

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
void HealthComponent::HandlePercentHealItemEvent(const PercentHealItemEvent& p_event) {
    this->Heal(p_event.fHealAmt * m_cap);
}

void HealthComponent::HandleFlatHealItemEvent(const FlatHealItemEvent& p_event) {
    this->Heal(p_event.fHealAmt);
}