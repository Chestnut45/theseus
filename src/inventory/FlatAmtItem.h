#pragma once

//-----------------------------------------------------------------------------
// File:            FlatAmtItem.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item which changes a given attribute
// by a flat amount (i.e. not a percentage or over time)
//-----------------------------------------------------------------------------

#include "ConsumableItem.h"
#include "W_EventManager.h"

struct FlatHealItemEvent {
    float fHealAmt;
};

struct FlatStaminaItemEvent {
    float fStaminaAmt;
};

class FlatAmtItem : public ConsumableItem {
    public:
        FlatAmtItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, const std::string& p_strImgPath, int p_iNumUses, Attribute p_enAttrib, float p_fAmt)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_strImgPath, p_iNumUses), m_enAttrib(p_enAttrib), m_fAmt(p_fAmt) {};

        ~FlatAmtItem() {};

        Attribute GetAttribute() const {return m_enAttrib;};
        void SetAttribute(Attribute p_enAttrib) {m_enAttrib = p_enAttrib;};

        float GetAmount() const {return m_fAmt;};
        void SetAmount(float p_fAmt) {m_fAmt = p_fAmt;};

        void Use();

    private:
        Attribute m_enAttrib;
        float m_fAmt;
};