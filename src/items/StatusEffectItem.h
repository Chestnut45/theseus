#pragma once

//-----------------------------------------------------------------------------
// File:            StatusEffectItem.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item which causes Theseus to incur a given
// status effect
//-----------------------------------------------------------------------------

#include "ConsumableItem.h"
#include "W_EventManager.h"
#include "StatusComponent.h"

class StatusEffectItem : public ConsumableItem {
    public:
        StatusEffectItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, Rarity p_enRarity, int p_iNumUses, StatusComponent::StatusEffectType p_enStatusEffectType, float p_fDuration)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex, p_enRarity, p_iNumUses), m_enType(p_enStatusEffectType), m_fDuration(p_fDuration) {
                this->SetConsumableType(STATUS_EFFECT);
            };

        ~StatusEffectItem() {};

        // Delete copy constructor/assignment
        StatusEffectItem(const StatusEffectItem&) = delete;
        StatusEffectItem& operator=(const StatusEffectItem&) = delete;

        // Delete move constructor/assignment
        StatusEffectItem(StatusEffectItem&& other) = delete;
        StatusEffectItem& operator=(StatusEffectItem&& other) = delete;

        virtual void Use();

        // Constructs a string containing all of the item details that will be displayed in the on-hover GUI tooltip
        inline virtual std::string GetToolTipText() const {
            // Get the base tooltip text...
            std::string strBaseText = ConsumableItem::GetToolTipText() + "\nApplies ";

            // ...then retrieve the name of the status effect this item applies...
            switch (m_enType) {
                case StatusComponent::BURNING:
                    strBaseText += "BURNING ";
                break;

                case StatusComponent::PETRIFIED:
                    strBaseText += "PETRIFIED ";
                break;

                case StatusComponent::POISONED:
                        strBaseText += "POISONED ";
                break;
            }

            // ...and the duration of the effect
            strBaseText += "for " + std::format("{:.2f}", m_fDuration);

            // Then return the constructed string
            return strBaseText;
        };

    private:
        StatusComponent::StatusEffectType m_enType;
        float m_fDuration;
};