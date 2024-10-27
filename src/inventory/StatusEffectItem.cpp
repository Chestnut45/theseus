#include "StatusEffectItem.h"

void StatusEffectItem::Use() {
    wolf::EventManager::EnqueueEvent(ApplyStatusEffectEvent(m_enType, m_fDuration));

    // Check if that was our last use of the item
    m_iNumUses--;
    if (m_iNumUses <= 0) {
        // And if it was, delete the item
        delete this;
    }
}