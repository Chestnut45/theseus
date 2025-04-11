#include "StatusEffectItem.h"

//-----------------------------------------------------------------------------
// File:            StatusEffectItem.cpp
// Original Author: Aurora Ryder
//
// A class representing a consumable item which causes Theseus to incur a given
// status effect
//-----------------------------------------------------------------------------

// Uses the item by sending off a ApplyStatusEffectEvent
void StatusEffectItem::Use() {
    wolf::EventManager::EnqueueEvent(ApplyStatusEffectEvent(m_enType, m_fDuration));

    // Check if that was our last use of the item
    m_iNumUses--;
    if (m_iNumUses <= 0) {
        // And if it was, delete the item
        delete this;
    }
}