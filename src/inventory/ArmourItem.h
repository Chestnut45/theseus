#pragma once

//-----------------------------------------------------------------------------
// File:            ArmourItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of armour
//-----------------------------------------------------------------------------

#include "EquipmentItem.h"
#include "../components/StatusComponent.h"
#include <vector>

struct ArmourStatusEffect {
    StatusComponent::StatusEffectType enType;
    float fDuration;
};

class ArmourItem : public EquipmentItem {
    public:
        ArmourItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, EquipmentSlot p_enSlot, float p_fDamageReduction, const std::vector<ArmourStatusEffect>& p_vStatusEffects)
            : EquipmentItem(p_enID, p_strName, p_strDesc, p_iValue, p_iTextureFrameIndex, p_enSlot), m_fDamageReduction(p_fDamageReduction), m_vStatusEffects(p_vStatusEffects) {};
        
        // Overload the SetEquipped method to send off an ArmourEquippedEvent
        virtual void SetEquipped(bool p_bEquip);

        void SetDamageReduction(float p_fReduction) {m_fDamageReduction = p_fReduction;};
        float GetDamageReduction() const {return m_fDamageReduction;};

        std::vector<ArmourStatusEffect>* GetStatusEffectList() {return &m_vStatusEffects;};

    private:
        float m_fDamageReduction; // How much damage does this piece of armour reduce (as a percentage)
        std::vector<ArmourStatusEffect> m_vStatusEffects;
};

// Event for when we equip a piece of armour
struct ArmourEquippedEvent {
    // Whatever armour we just equipped
    ArmourItem* pArmour;
};