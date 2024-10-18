#pragma once

//-----------------------------------------------------------------------------
// File:            PercentItem.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item which changes a given attribute
// by a percentage of its maximum value
//-----------------------------------------------------------------------------

#include "ConsumableItem.h"
#include "W_EventManager.h"

class PercentItem : public ConsumableItem {
    public:
        PercentItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iNumUses, Attribute p_enAttrib, float p_fAmt)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iNumUses), m_enAttrib(p_enAttrib), m_fAmt(p_fAmt) {
                this->SetConsumableType(PERCENT_AMT);
            };

        ~PercentItem() {};

        Attribute GetAttribute() const {return m_enAttrib;};
        void SetAttribute(Attribute p_enAttrib) {m_enAttrib = p_enAttrib;};

        float GetAmount() const {return m_fAmt;};
        void SetAmount(float p_fAmt) {m_fAmt = p_fAmt;};

        virtual void Use();

    private:
        Attribute m_enAttrib;
        float m_fAmt;
};