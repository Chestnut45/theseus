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
        ArmourItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, EquipmentSlot p_enSlot, float p_fDamageReduction, StatusComponent* p_pStatusComponent)
            : EquipmentItem(p_enID, p_strName, p_strDesc, p_iValue, p_iTextureFrameIndex, p_enSlot), m_fDamageReduction(p_fDamageReduction), m_pStatusComponent(p_pStatusComponent) {};
        
        // Overload the SetEquipped method to send off an ArmourEquippedEvent
        virtual void SetEquipped(bool p_bEquip);

        void SetDamageReduction(float p_fReduction) {m_fDamageReduction = p_fReduction;};
        float GetDamageReduction() const {return m_fDamageReduction;};

        void SetStatusComponent(StatusComponent* p_pComponent) {m_pStatusComponent = p_pComponent;};
        StatusComponent* GetStatusComponent() const {return m_pStatusComponent;};

    private:
        float m_fDamageReduction; // How much damage does this piece of armour reduce (as a percentage)
        StatusComponent* m_pStatusComponent = nullptr; // Status component that lets us add status effects (if we want to!)
};

// Event for when we equip a piece of armour
struct ArmourEquippedEvent {
    // Whatever armour we just equipped
    ArmourItem* pArmour;
};