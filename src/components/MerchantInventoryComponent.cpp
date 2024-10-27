#include "MerchantInventoryComponent.h"

MerchantInventoryComponent::~MerchantInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister this event's listeners
    wolf::EventManager::RemoveListener<OpenInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleOpenInventoryEvent>(*this);
}

bool MerchantInventoryComponent::StockMerchantFromFile(const std::string& p_strFilePath) {

}

void MerchantInventoryComponent::Open() {
    m_bIsOpen = true;

    wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
}

void MerchantInventoryComponent::Close() {
    m_bIsOpen = false;

    wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
}

void MerchantInventoryComponent::ToggleOpen() {
    m_bIsOpen = !m_bIsOpen;

    if (m_bIsOpen) {
        wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
    }
    else {
        wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
    }
}

// This method uses the same exact structure as the base class AddItem(ItemBase* p_pItem) method, but
// it does not add the item to the inventory, it only checks if the item *can* be added
bool MerchantInventoryComponent::CanAddItem(ItemBase* p_pItem) {
    // If we're not using any of the slots then we can just insert the item so we return true
    if (m_iSlotsInUse == 0) {
        return true;
    }

    // Otherwise we need to look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same ID and it is stackable
        if (pItem->GetID() == p_pItem->GetID() && p_pItem->IsStackable() && pItem->IsStackable()) {
            // Then we've got room for the item we want to add so we return true
            return true;
        }
    }

    // Otherwise, we check if there is an empty inventory slot to start a new item stack in
    if (m_iSlotsInUse < m_iSize) {
        // And return true if there is one
        return true;
    }

    // And if all that fails, then we can't fit the item so we return false
    return false;
}

void MerchantInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this merchant is open
    if (m_bIsOpen) {
        // And another merchant just opened
        if (p_event.enType == MERCHANT_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
        }
    }
}

void MerchantInventoryComponent::HandleSellItemToMerchantEvent(const SellItemToMerchantEvent& p_event) {
    // If the player is selling an item to this specific merchant
    if (p_event.iMerchantIdNum == m_iIdNum) {
        // If there is enough room in the merchant's inventory for the item
        if (CanAddItem(p_event.pItem)) {
            // If the merchant has enough gold to buy the item
            if (p_event.pItem->GetValue() <= m_iGold) {
                m_iGold -= p_event.pItem->GetValue(); // Take the gold from the merchant
                this->AddItem(p_event.pItem); // And add the item to their inventory

                // Then let the player know that the item has been sold
                wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(p_event.pItem->GetName(), p_event.iPlayerInventoryIndex, true));
            }
            else { // If the merchant does not have enough gold
                m_bShowSellForLessPrompt = true; // We need to ask the player if they are willing to sell the item, anyway

                // So we put the item that we're asking about in "stasis" so we can keep track of it outside of this method
                m_ItemInStasis.pItem = p_event.pItem;
                m_ItemInStasis.iInventoryIndex = p_event.iPlayerInventoryIndex;
            }
        }
    }
}
void MerchantInventoryComponent::HandleBuyItemFromMerchantEvent(const BuyItemFromMerchantEvent& p_event) {
    if (p_event.iMerchantIdNum == m_iIdNum) {

    }
}

void MerchantInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {

    }
}