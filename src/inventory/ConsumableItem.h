#pragma once

//-----------------------------------------------------------------------------
// File:            ConsumableItem.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item
//-----------------------------------------------------------------------------

#include "ItemBase.h"

enum Attribute {
    HEALTH = 1,
    STAMINA = 2
};

enum ConsumableType {
    BASIC_CONSUMABLE = 1,
    FLAT_AMT = 2,
    PERCENT_AMT = 4,
    STATUS_EFFECT = 8,
};

class ConsumableItem : public ItemBase {
    public:
        ConsumableItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, int p_iNumUses)
            : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex), m_iNumUses(p_iNumUses){};
        
        ~ConsumableItem() {};

        int GetNumUses() const {return m_iNumUses;};
        void SetNumUses(int p_iNumUses) {m_iNumUses = p_iNumUses;};

        ConsumableType GetConsumableType() {return m_enType;};

        virtual void Use() {
            m_iNumUses--;
            if (m_iNumUses <= 0) {
                this->~ConsumableItem();
            }
        };

    protected:
        void SetConsumableType(ConsumableType p_enType) {m_enType = p_enType;};

        int m_iNumUses;
        ConsumableType m_enType = BASIC_CONSUMABLE;
};