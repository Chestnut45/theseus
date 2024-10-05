#pragma once

//-----------------------------------------------------------------------------
// File:            EquipmentItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of equipment (armor/weapon)
//-----------------------------------------------------------------------------

#include "ItemBase.h"

enum EquipmentSlot {
    WEAPON = 1,
    HEAD = 2,
    BODY = 4,
    ARMS = 8,
    LEGS = 16,
    FEET = 32,
    GLOVES = 64,
    ACCESSORY = 128
};

class EquipmentItem : public ItemBase {
    public:
        EquipmentItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, const std::string& p_strImgPath, EquipmentSlot p_enEquipmentSlot)
            : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_strImgPath), m_enSlot(p_enEquipmentSlot) {};

        ~EquipmentItem();

        bool IsEquipped() {return m_bEquipped;};
        void SetEquipped(bool p_bEquip) {m_bEquipped = p_bEquip;};

        // Once you set the equipment slot you can't change it later
        EquipmentSlot GetEquipmentSlot() {return m_enSlot;};

    private:
        bool m_bEquipped = false;
        EquipmentSlot m_enSlot;
};