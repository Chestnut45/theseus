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

class StatusEffectItem : public ConsumableItem {
    public:
        StatusEffectItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iNumUses)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iNumUses) {};

        ~StatusEffectItem() {};

        void Use();

    private:
        
};