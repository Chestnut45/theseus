#pragma once

//-----------------------------------------------------------------------------
// File:            ArmourItem.h
// Original Author: Aurora Ryder
// Modifications: Nguyễn Minh Nhật
// A class representing the item version of a piece of armour
//-----------------------------------------------------------------------------

#include <vector>

#include "EquipmentItem.h"
#include <StatusComponent.h>

// Struct for status effects that can be applied via ArmourItems
struct ArmourStatusEffect {
    StatusComponent::StatusEffectType enType;
    float fDuration;
};

class ArmourItem : public EquipmentItem {
    public:
        ArmourItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, Rarity p_enRarity, EquipmentSlot p_enSlot, float p_fDamageReduction, const std::vector<ArmourStatusEffect>& p_vStatusEffects, float p_aStatusEffectResistances[StatusComponent::StatusEffectType::NONE])
            : EquipmentItem(p_enID, p_strName, p_strDesc, p_iValue, p_iTextureFrameIndex, p_enRarity, p_enSlot), m_fDamageReduction(p_fDamageReduction), m_vStatusEffects(p_vStatusEffects) 
            {
                for(int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
                {
                    m_aStatusEffectResistances[i] = p_aStatusEffectResistances[i];
                }
            };
        
        // Overload the SetEquipped method to send off an ArmourEquippedEvent
        virtual void SetEquipped(bool p_bEquip);

        void SetDamageReduction(float p_fReduction) {m_fDamageReduction = p_fReduction;};
        float GetDamageReduction() const {return m_fDamageReduction;};

        std::vector<ArmourStatusEffect>* GetStatusEffectList() {return &m_vStatusEffects;};
        const float* GetStatusEffectResistances() const {return m_aStatusEffectResistances;};

        // Constructs a string containing all of the item details that will be displayed in the on-hover GUI tooltip
        inline virtual std::string GetToolTipText() const {
            // Get the basic tooltip text and add the defense stat
            std::string strBaseText = EquipmentItem::GetToolTipText() + "\nDefense: " + std::format("{:.2f}", m_fDamageReduction);
            
            // If this item applies status effects
            if (!m_vStatusEffects.empty()) {
                // Iterate through the list of status effects...
                strBaseText += "\nApplies: ";
                for (auto& effect : m_vStatusEffects) {
                    // ...and add their name...
                    switch (effect.enType) {
                        case StatusComponent::BURNING:
                            strBaseText += "\n\tBURNING ";
                        break;

                        case StatusComponent::PETRIFIED:
                            strBaseText += "\n\tPETRIFIED ";
                        break;

                        case StatusComponent::POISONED:
                            strBaseText += "\n\tPOISONED ";
                        break;

                        case StatusComponent::HEALING:
                            strBaseText += "\n\tHEALING ";
                        break;
                    }

                    // ...and durations
                    strBaseText += "for " + std::format("{:.2f}", effect.fDuration);
                }
            }
            
            // Then return the constructed string
            return strBaseText;
        };

    private:
        float m_fDamageReduction; // How much damage does this piece of armour reduce (as a percentage)
        std::vector<ArmourStatusEffect> m_vStatusEffects;

        //-----------------//
        //                 //
        //  Added by Nhat  //
        //                 //
        //-----------------//
        float m_aStatusEffectResistances [StatusComponent::StatusEffectType::NONE];    // Resistance to status effect values
};

// Event for when we equip a piece of armour
struct ArmourEquippedEvent {
    // Whatever armour we just equipped
    ArmourItem* pArmour;
};
// Event for when we unequip a piece of armour
struct ArmourUnequippedEvent {
    // Whatever armour we just equipped
    ArmourItem* pArmour;
};