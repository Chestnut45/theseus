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
        ConsumableItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, const std::string& p_strImgPath, int p_iCount, int p_iNumUses) :
        ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_strImgPath), m_iCount(p_iCount), m_iNumUses(p_iNumUses){};
        
        ~ConsumableItem() {};

        int GetCount() const {return m_iCount;};
        void SetCount(int p_iCount) {m_iCount = p_iCount;};
        void IncreaseCount(int p_iAmt) {m_iCount += p_iAmt;};
        void DecreaseCount(int p_iAmt) {m_iCount -= p_iAmt;};

        int GetNumUses() const {return m_iNumUses;};
        void SetNumUses(int p_iNumUses) {m_iNumUses = p_iNumUses;};

        virtual void UseItem() {
            if (m_iNumUses > 0) {
                m_iNumUses--;
            }

            if (m_iNumUses <= 0) {
                m_iCount--;
                if (m_iCount <= 0) {
                    this->~ConsumableItem();
                }
            }
        };

    private:
        int m_iCount;
        int m_iNumUses;

};