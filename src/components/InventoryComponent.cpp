#include "InventoryComponent.h"

InventoryComponent::InventoryComponent(int p_iSize) : m_iSize(p_iSize) {
    // Reserve the amount of space we've been asked for
    m_vvpContents.reserve(p_iSize);
}

InventoryComponent::~InventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();
}

void InventoryComponent::Update(float p_fDelta) {
    // Do cool and epic things B]
}

ItemBase* InventoryComponent::GetItem(const std::string& p_strItemName) {
    // Look through the inventory contents
    for (std::vector<std::stack<ItemBase*>>::iterator it = m_vvpContents.begin(); it != m_vvpContents.end(); ++it) {
        // Inventory "slots" are vectors whose items are all the same so we can just look at the
        // first item in the subvectors when we're looking for something
        ItemBase* pItem = it->top();

        // If we find an item with the same name as what we're looking for
        if (strcmp(pItem->GetName().c_str(), p_strItemName.c_str()) == 0) {
            return pItem; // We return it!
        }
    }
    // Otherwise, return a null pointer
    return nullptr;
}

// Follows the same process as GetItem with the item name as the parameter
ItemBase* InventoryComponent::GetItem(ItemID p_enItemID) {
    // Look through the inventory contents
    for (std::vector<std::stack<ItemBase*>>::iterator it = m_vvpContents.begin(); it != m_vvpContents.end(); ++it) {
        // Inventory "slots" are vectors whose items are all the same so we can just look at the
        // first item in the subvectors when we're looking for something
        ItemBase* pItem = it->top();

        // If we find an item with the same ID
        if (pItem->GetID() == p_enItemID) {
            return pItem; // We return it!
        }
    }
    // Otherwise, return a null pointer
    return nullptr;
}

// This GetItem method is for when we know the index (inventory "slot" number) of the item we're looking for
ItemBase* InventoryComponent::GetItem(int p_iItemIndex) {
    if (p_iItemIndex > m_vvpContents.size() || p_iItemIndex < 0) {
        // That's an invalid index so we can't get the item there
        return nullptr;
    }

    // By default we return the first item in a "stack" of items
    return m_vvpContents.at(p_iItemIndex).top();
}

bool InventoryComponent::AddItem(ItemBase* p_pItem) {
    // Look through the inventory contents
    for (std::vector<std::stack<ItemBase*>>::iterator it = m_vvpContents.begin(); it != m_vvpContents.end(); ++it) {
        // Inventory "slots" are vectors whose items are all the same so we can just look at the
        // first item in the subvectors when we're looking for something
        ItemBase* pItem = it->top();

        // If we find an item with the same ID and it is stackable
        if (pItem->GetID() == p_pItem->GetID() && p_pItem->IsStackable()) {
            // Then we push the item to the stack
            it->push(p_pItem);
            return true;
        }
    }

    // Otherwise, we check if there is an empty inventory slot to start a new item stack in
    if (m_iSlotsInUse < m_iSize) {
        // Push the item to the next open slot
        m_vvpContents.at(m_iSlotsInUse + 1).push(p_pItem);

        // Update the number of slots we're using
        m_iSlotsInUse++;

        // And return true
        return true;
    }

    // And if all that fails, we return false
    return false;
}

void InventoryComponent::EmptyInventory() {
    // Go through the contents vector
    for (std::vector<std::stack<ItemBase*>>::iterator it = m_vvpContents.begin(); it != m_vvpContents.end(); ++it) {
        // And empty each of the item stacks within
        while (!it->empty()) {
            ItemBase* pNextItem = it->top();
            it->pop();
            delete(pNextItem);
        }
    }
}

