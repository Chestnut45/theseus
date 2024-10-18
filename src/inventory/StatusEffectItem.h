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
        StatusEffectItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, int p_iNumUses, StatusComponent::StatusEffectType p_enStatusEffectType, float p_fDuration)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex, p_iNumUses), m_enType(p_enStatusEffectType), m_fDuration(p_fDuration) {
                this->SetConsumableType(STATUS_EFFECT);
            };

        ~StatusEffectItem() {};

        virtual void Use();

    private:
        StatusComponent::StatusEffectType m_enType;
        float m_fDuration;
};