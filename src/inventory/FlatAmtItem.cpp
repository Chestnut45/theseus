#include "FlatAmtItem.h"

void FlatAmtItem::Use() {
    // Figure out which attribute this item is affecting and send the corresponding event
    switch(this->m_enAttrib) {
        case HEALTH:
            wolf::EventManager::EnqueueEvent(FlatHealthItemEvent(this->m_fAmt));
        break;

        case STAMINA:
            wolf::EventManager::EnqueueEvent(FlatStaminaItemEvent(this->m_fAmt));
        break;
    }

    // Check if that was our last use of the item
    m_iNumUses--;
    if (m_iNumUses <= 0) {
        // And if it was, delete the item
        delete this;
    }
}