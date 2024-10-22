#pragma once

//-----------------------------------------------------------------------------
// File:            EquipmentItem.h
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of equipment (armor/weapon)
//-----------------------------------------------------------------------------

#include "ItemBase.h"
#include "StatusComponent.h"

enum EquipmentSlot {
    WEAPON,
    HEAD,
    BODY,
    ARMS,
    LEGS,
    FEET,
    GLOVES,
    ACCESSORY,
    END_OF_EQUIPMENT, // Sentinel value for iteration
};

enum ArmourType
{
    SPIKEDHELMET,
    SPIKEDPLATES,
    SPIKEDARMS,
    SPIKEDPANTS,
    SPIKEDBOOTS,
    SPIKEDGLOVES
};

class EquipmentItem : public ItemBase {
    public:
        

        // Note that EquipmentItems CANNOT be stacked
        EquipmentItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, int p_iTextureFrameIndex, EquipmentSlot p_enEquipmentSlot)
            : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, false, p_iTextureFrameIndex), m_enSlot(p_enEquipmentSlot)
        {
            // It is useful to have a string representation of the EquipmentSlot enum,
            // so this switch case sets that up automatically when an item is created
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

        // Delete copy constructor/assignment
        EquipmentItem(const EquipmentItem&) = delete;
        EquipmentItem& operator=(const EquipmentItem&) = delete;

        // Delete move constructor/assignment
        EquipmentItem(EquipmentItem&& other) = delete;
        EquipmentItem& operator=(EquipmentItem&& other) = delete;

        bool IsEquipped() {return m_bEquipped;};
        virtual void SetEquipped(bool p_bEquip) {m_bEquipped = p_bEquip;};

        // Once you set the equipment slot you can't change it later
        EquipmentSlot GetEquipmentSlot() {return m_enSlot;};
        int GetDetailedType(){return m_enDetailedType;};

        // This method should ONLY be used when you want to print or otherwise display the equipment slot
        // for all other uses such as comparison/iteration/etc. use GetEquipmentSlot() and the enum itself.
        const std::string& GetEquipmentSlotString() {return m_strSlot;};

    protected:
        bool m_bEquipped = false;
        EquipmentSlot m_enSlot;
        std::string m_strSlot;
        int m_enDetailedType;
};