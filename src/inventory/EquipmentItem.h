#pragma once

//-----------------------------------------------------------------------------
// File:            EquipmentItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of equipment (armor/weapon)
//-----------------------------------------------------------------------------

#include "ItemBase.h"

enum EquipmentSlot {
    WEAPON,
    HEAD,
    BODY,
    ARMS,
    LEGS,
    FEET,
    GLOVES,
    ACCESSORY
};

class EquipmentItem : public ItemBase {
    public:
        EquipmentItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, const std::string& p_strImgPath, EquipmentSlot p_enEquipmentSlot)
            : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_strImgPath), m_enSlot(p_enEquipmentSlot)
        {
            switch (m_enSlot) {
                case WEAPON:
                    m_strSlot = "WEAPON";
                break;

                case HEAD:
                    m_strSlot = "HEAD";
                break;

                case BODY:
                    m_strSlot = "BODY";
                break;

                case ARMS:
                    m_strSlot = "ARMS";
                break;

                case LEGS:
                    m_strSlot = "LEGS";
                break;

                case FEET:
                    m_strSlot = "FEET";
                break;

                case GLOVES:
                    m_strSlot = "GLOVES";
                break;

                case ACCESSORY:
                    m_strSlot = "ACCESSORY";
                break;
                
                default:
                    m_strSlot = "PROBLEM!";
                break;
            }
        };

        ~EquipmentItem() {};

        bool IsEquipped() {return m_bEquipped;};
        void SetEquipped(bool p_bEquip) {m_bEquipped = p_bEquip;};

        // Once you set the equipment slot you can't change it later
        EquipmentSlot GetEquipmentSlot() {return m_enSlot;};

        const std::string& GetEquipmentSlotString() {return m_strSlot;};

    private:
        bool m_bEquipped = false;
        EquipmentSlot m_enSlot;
        std::string m_strSlot;
};