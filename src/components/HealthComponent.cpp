//-----------------------------------------------------------------------------
// File: HealthComponent.cpp
// Original Author: Nguyễn Minh Nhật
// Health.
//-----------------------------------------------------------------------------

#include "HealthComponent.h"
#include "PlayerInventoryComponent.h"
#include "../inventory/ArmourItem.h"

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
        float finalDamage = p_damage;
        float damageReduction = 0.0f;

        // Get armour for damage reduction
        PlayerInventoryComponent* pic = this->GetGameObject()->GetComponent<PlayerInventoryComponent>();
        if(pic != nullptr)
        {
            ArmourItem* headgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::HEAD));
            damageReduction += headgear != nullptr ? headgear->GetDamageReduction() : 0;

            ArmourItem* bodygear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::BODY));
            damageReduction += bodygear != nullptr ? bodygear->GetDamageReduction() : 0;

            ArmourItem* armsgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::ARMS));
            damageReduction += armsgear != nullptr ? armsgear->GetDamageReduction() : 0;

            ArmourItem* glovesgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::GLOVES));
            damageReduction += glovesgear != nullptr ? glovesgear->GetDamageReduction() : 0;

            ArmourItem* legsgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::LEGS));
            damageReduction += legsgear != nullptr ? legsgear->GetDamageReduction() : 0;

            ArmourItem* feetgear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::FEET));
            damageReduction += feetgear != nullptr ? feetgear->GetDamageReduction() : 0;

            ArmourItem* accessorygear = static_cast<ArmourItem*>(pic->GetEquippedItem(EquipmentSlot::ACCESSORY));
            damageReduction += accessorygear != nullptr ? accessorygear->GetDamageReduction() : 0;

            // std::cout << "HealthComponent - damred: " << damageReduction << std::endl;
        }


        this->m_health -= p_damage * (1.0f - damageReduction);

        if (m_health < 0) m_health = 0;


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

// Reduce health & ignore armour
void HealthComponent::Pierce(float p_damage)
{
    if(this->m_health > 0)
    {
        this->m_health -= p_damage;
        if (m_health < 0) m_health = 0;
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