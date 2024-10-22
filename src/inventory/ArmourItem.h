#pragma once

//-----------------------------------------------------------------------------
// File:            ArmourItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of armour
//-----------------------------------------------------------------------------

#include "EquipmentItem.h"
#include "../components/StatusComponent.h"

class ArmourItem : public EquipmentItem {
    public:
        ArmourItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, EquipmentSlot p_enSlot, float p_fDamageReduction, StatusComponent::StatusEffectType p_enStatusEffect, float p_fDuration)
            : EquipmentItem(p_enID, p_strName, p_strDesc, p_iValue, p_iTextureFrameIndex, p_enSlot), m_fDamageReduction(p_fDamageReduction), m_enStatusEffect(p_enStatusEffect), m_fDuration(p_fDuration) {
                if (p_enStatusEffect != StatusComponent::StatusEffectType::NONE) {
                    m_pStatusComponent = new StatusComponent();
                    m_pStatusComponent->AddStatusEffect(p_enStatusEffect, p_fDuration);
                }
            };
        
        // Overload the SetEquipped method to send off an ArmourEquippedEvent
        virtual void SetEquipped(bool p_bEquip);

        void SetDamageReduction(float p_fReduction) {m_fDamageReduction = p_fReduction;};
        float GetDamageReduction() const {return m_fDamageReduction;};

        StatusComponent* GetStatusComponent() const {return m_pStatusComponent;};

        StatusComponent::StatusEffectType GetStatusEffectType() const {return m_enStatusEffect;};
        bool SetStatusEffectType(StatusComponent::StatusEffectType p_enType) {
            // If this item is supposed to have status effects
            if (m_pStatusComponent) {
                // Then add a new one with the new type
                m_pStatusComponent->AddStatusEffect(p_enType, m_fDuration);
                m_enStatusEffect = p_enType;
                // !-- I feel like we need to be able to remove the previous status effect --!
                return true;
            }

            return false;
        }
        
        void SetStatusEffectDuration(float p_fDuration) {m_fDuration = p_fDuration;};
        float GetStatusEffectDuration() const {return m_fDuration;};

    private:
        float m_fDamageReduction; // How much damage does this piece of armour reduce (as a percentage)
        StatusComponent* m_pStatusComponent = nullptr; // Status component that lets us add status effects (if we want to!)

        StatusComponent::StatusEffectType m_enStatusEffect = StatusComponent::StatusEffectType::NONE;
        float m_fDuration = 0.0f;
};

// Event for when we equip a piece of armour
struct ArmourEquippedEvent {
    // Whatever armour we just equipped
    ArmourItem* pArmour;
};