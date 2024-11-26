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
        PercentItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, Rarity p_enRarity, int p_iNumUses, Attribute p_enAttrib, float p_fAmt)
            : ConsumableItem(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex, p_enRarity, p_iNumUses), m_enAttrib(p_enAttrib), m_fAmt(p_fAmt) {
                this->SetConsumableType(PERCENT_AMT);
            };

        ~PercentItem() {};

        // Delete copy constructor/assignment
        PercentItem(const PercentItem&) = delete;
        PercentItem& operator=(const PercentItem&) = delete;

        // Delete move constructor/assignment
        PercentItem(PercentItem&& other) = delete;
        PercentItem& operator=(PercentItem&& other) = delete;

        Attribute GetAttribute() const {return m_enAttrib;};
        void SetAttribute(Attribute p_enAttrib) {m_enAttrib = p_enAttrib;};

        float GetAmount() const {return m_fAmt;};
        void SetAmount(float p_fAmt) {m_fAmt = p_fAmt;};

        virtual void Use();

        inline virtual std::string GetToolTipText() const {
            std::string strBaseText = ConsumableItem::GetToolTipText() + "\nEffect: ";
            switch(m_enAttrib) {
                case HEALTH:
                    strBaseText += "HEALTH ";
                break;

                case STAMINA:
                    strBaseText += "STAMINA ";
                break;
            }
            if (m_fAmt > 0) {
                strBaseText += "+ " + std::format("{:.2f}", m_fAmt * 100) + "%";
            }
            else {
                strBaseText += "- " + std::format("{:.2f}", abs(m_fAmt * 100)) + "%";
            }
            return strBaseText;
        };

    private:
        Attribute m_enAttrib;
        float m_fAmt;
};