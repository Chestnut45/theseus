#include "InventoryComponent.h"
#include <imgui/imgui.h>

InventoryComponent::InventoryComponent(int p_iSize, int p_iSlotsPerRow) : m_iSize(p_iSize), m_iMaxPerRow(p_iSlotsPerRow) {
    // Reserve the amount of space we've been asked for
    m_vvpContents.reserve(p_iSize);

    // And fill it up with empty stacks
    for (int i = 0; i < p_iSize; i++) {
        std::stack<ItemBase*> stack;
        m_vvpContents.push_back(stack);
    }
}

InventoryComponent::~InventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();
}

ItemBase* InventoryComponent::GetItem(const std::string& p_strItemName) {
    // If there are no slots in use then we can't return anything!
    if (m_iSlotsInUse == 0) {
        return nullptr;
    }

    // Look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

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
    // If there are no slots in use then we can't return anything!
    if (m_iSlotsInUse == 0) {
        return nullptr;
    }

    // Look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same name as what we're looking for
        if (pItem->GetID() == p_enItemID) {
            return pItem; // We return it!
        }
    }

    // Otherwise, return a null pointer
    return nullptr;
}

// This GetItem method is for when we know the index (inventory "slot" number) of the item we're looking for
ItemBase* InventoryComponent::GetItem(int p_iItemIndex) {
    if (p_iItemIndex > m_vvpContents.size() || p_iItemIndex < 0 || p_iItemIndex >= m_iSlotsInUse) {
        // That's an invalid index so we can't get the item there
        return nullptr;
    }

    // By default we return the first item in a "stack" of items
    return m_vvpContents[p_iItemIndex].top();
}

bool InventoryComponent::AddItem(ItemBase* p_pItem) {
    // If we're not using any of the slots then we can just insert the item
    if (m_iSlotsInUse == 0) {
        m_vvpContents[m_iSlotsInUse].push(p_pItem);
        m_iSlotsInUse++;
        return true;
    }

    // Otherwise we need to look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same ID and it is stackable
        if (pItem->GetID() == p_pItem->GetID() && p_pItem->IsStackable()) {
            // Then we push the item to the stack
            m_vvpContents[i].push(p_pItem);
            return true;
        }
    }

    // Otherwise, we check if there is an empty inventory slot to start a new item stack in
    if (m_iSlotsInUse < m_iSize) {
        // Push the item to the next open slot
        m_vvpContents[m_iSlotsInUse].push(p_pItem);

        // Update the number of slots we're using
        m_iSlotsInUse++;

        // And return true
        return true;
    }

    // And if all that fails, we return false
    return false;
}

// Note that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(const std::string& p_strItemName) {
    // If our inventory is empty then we can't remove anything!
    if (m_iSlotsInUse == 0) {
        return false;
    }

    for (int p = 0; p < m_iSlotsInUse; p++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[p].top();

        // If we find an item with the same name as what we're looking for
        if (strcmp(pItem->GetName().c_str(), p_strItemName.c_str()) == 0) {
            // Then we remove it
            m_vvpContents[p].pop();

            // And check if that stack is now empty
            if (m_vvpContents[p].empty()) {
                // If it is, then we need to decrease the number of slots in use
                m_iSlotsInUse--;

                // And reorganize the vector to fill the gap by removing the empty stack from the vector
                std::stack<ItemBase*> pEmptyStack = m_vvpContents[p];
                m_vvpContents.erase(m_vvpContents.begin() + p);

                // And then pushing it back onto the vector
                m_vvpContents.push_back(pEmptyStack);
            }

            // And return true
            return true;
        }
    }

    // Otherwise, we couldn't remove the item so we return false
    return false;
}

// Once again, note that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(ItemID p_enItemID) {
    // If our inventory is empty then we can't remove anything!
    if (m_iSlotsInUse == 0) {
        return false;
    }

    for (int p = 0; p < m_iSlotsInUse; p++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[p].top();

        // If we find an item with the same ID as what we're looking for
        if (pItem->GetID() == p_enItemID) {
            // Then we remove it
            m_vvpContents[p].pop();

            // And check if that stack is now empty
            if (m_vvpContents[p].empty()) {
                // If it is, then we need to decrease the number of slots in use
                m_iSlotsInUse--;

                // And reorganize the vector to fill the gap by removing the empty stack from the vector
                std::stack<ItemBase*> pEmptyStack = m_vvpContents[p];
                m_vvpContents.erase(m_vvpContents.begin() + p);

                // And then pushing it back onto the vector
                m_vvpContents.push_back(pEmptyStack);
                
            }

            // And return true
            return true;
        }
    }

    // Otherwise, we couldn't remove the item so we return false
    return false;
}

// Keep in mind that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(int p_iItemIndex) {
    if (p_iItemIndex > m_vvpContents.size() || p_iItemIndex < 0 || p_iItemIndex >= m_iSlotsInUse) {
        // If we're given an invalid index then we return false
        return false;
    }

    // Otherwise we remove the first item in the stack at the given index
    m_vvpContents[p_iItemIndex].pop();

    // And check if doing so emptied that stack
    if (m_vvpContents[p_iItemIndex].empty()) {
        // If so, we decrease the number of slots in use
        m_iSlotsInUse--;

        // And reorganize the vector to fill the gap by removing the empty stack from the vector
        std::stack<ItemBase*> pEmptyStack = m_vvpContents[p_iItemIndex];
        m_vvpContents.erase(m_vvpContents.begin() + p_iItemIndex);

        // And then pushing it back onto the vector
        m_vvpContents.push_back(pEmptyStack);
    }

    // And return true
    return true;
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

void InventoryComponent::DEBUGPrintInventory() {
    printf("Inventory Contents:\n");
    for (int q = 0; q < m_iSlotsInUse; q++) {
        ItemBase* pItem = m_vvpContents[q].top();
        printf("[Name: %s Quanity: %d] ", pItem->GetName().c_str(), m_vvpContents[q].size());
    }
    printf("\n");
}

void InventoryComponent::ShowInventoryGUI() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos({500, 200});
    ImGui::SetNextWindowSize({500, 500});
    ImGui::Begin("~ Inventory ~", nullptr, flags);

    int counter = 0;
    for (int k = 0; k < m_iSize; k++) {
        if (!m_vvpContents[k].empty()) {
            if (ImGui::Button(m_vvpContents[k].top()->GetName().c_str(), ImVec2(50, 50))) {
                
            }
        }
        else {
            if(ImGui::Button("##", ImVec2(50, 50))) {

            }
        }
        if (counter == m_iMaxPerRow - 1) {
            counter = 0;
        }
        else {
            ImGui::SameLine();
            counter++;
        }
    }
    // End of window
    ImGui::End();
}