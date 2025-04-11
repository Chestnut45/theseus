//-----------------------------------------------------------------------------
// File:            ArmourItem.cpp
// Original Author: Aurora Ryder
// Modifications: Nguyễn Minh Nhật
// A class representing the item version of a piece of armour
//-----------------------------------------------------------------------------

#include "ArmourItem.h"

//-----------------------------------------------------------------------------
// File:            ArmourItem.cpp
// Original Author: Aurora Ryder
//
// A class representing the item version of a piece of armour
//-----------------------------------------------------------------------------

// We want to send an armour equipped event anytime we equip armour so we overload the SetEquipped method
void ArmourItem::SetEquipped(bool p_bEquip) {
    // Equip or unequip the item
    m_bEquipped = p_bEquip;

    // If we just equipped it

    if (m_bEquipped) {
        // Let whoever is interested know
        wolf::EventManager::TriggerEvent(ArmourEquippedEvent(this));
    }
    // If we just unequipped it
    else
    {
        // Let whoever is interested know
        wolf::EventManager::TriggerEvent(ArmourUnequippedEvent(this));
    }
}