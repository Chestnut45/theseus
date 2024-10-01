#pragma once

//-----------------------------------------------------------------------------
// File:            ItemBase.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item
//-----------------------------------------------------------------------------

#include "ItemBase.h"

class ConsumableItem : public ItemBase {
    public:
        ConsumableItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, const std::string& p_strImgPath, int p_iNumUses)
            : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_strImgPath), m_iNumUses(p_iNumUses){};
        
        ~ConsumableItem() {};

        int GetNumUses() const {return m_iNumUses;};
        void SetNumUses(int p_iNumUses) {m_iNumUses = p_iNumUses;};

        virtual void UseItem() {
            m_iNumUses--;
            if (m_iNumUses <= 0) {
                this->~ConsumableItem();
            }
        };

    private:
        int m_iNumUses;

    friend class FlatAmtItem;
    friend class PercentItem;
    friend class StatusEffectItem;

};