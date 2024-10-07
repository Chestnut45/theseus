#include "InventoryComponent.h"
#include "W_Logging.h"
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

// !-- THIS METHOD SHOULD BE REMOVED LATER --!
void InventoryComponent::DEBUGPrintInventory() {
    printf("Inventory Contents:\n");
    for (int q = 0; q < m_iSlotsInUse; q++) {
        ItemBase* pItem = m_vvpContents[q].top();
        printf("[Name: %s Quanity: %d] ", pItem->GetName().c_str(), m_vvpContents[q].size());
    }
    printf("\n");
}

void InventoryComponent::ShowInventoryGUI() {
    // You can't resize the inventory but you can move it around!
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize;

    // By default, the inventory appears close to the middle of the screen
    ImGui::SetNextWindowPos({500, 200});
    ImGui::SetNextWindowSize({250, 250});
    ImGui::Begin("~ Inventory ~", nullptr, flags);

    // This counter lets us control how many items are drawn in a row
    int counter = 0;

    // We need to draw m_iSize number of slots
    for (int k = 0; k < m_iSize; k++) {
        // If there is an item (or stack of items as it were) in this slot
        if (!m_vvpContents[k].empty()) {
            // We grab a reference to the top item and create a variable to hold the item's details
            ItemBase* pItem = m_vvpContents[k].top();
            std::string strTooltipText;

            // There are different rules for drawing Consumables and Equipment Items so we need to figure out
            // what this particular item is before we go any further

            // There's a chance we won't need this value but if we do then we need it to survive the if ID == EQUIPMENT scope
            bool bIsEquipped = false;

            // If this is a consumable item
            if (pItem->GetID() == CONSUMABLE) {
                // Try to cast it
                ConsumableItem* pConsumable = static_cast<ConsumableItem*>(pItem);
                if (!pConsumable) {
                    // And throw an error if we couldn't
                    wolf::Error("Failed to cast ItemBase to ConsumableItem!\n");
                }

                // Then construct the string that will be used to display all of the item's details
                strTooltipText = pConsumable->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")\n\n" + pConsumable->GetDescription() 
                    + "\n\nValue: " + std::to_string(pConsumable->GetValue()) + "\nUses: " + std::to_string(pConsumable->GetNumUses());
            }
            else if (pItem->GetID() == EQUIPMENT) { // If this is an equipment item
                // Try to cast it
                EquipmentItem* pEquipment = static_cast<EquipmentItem*>(pItem);
                if (!pEquipment) {
                    // And throw an error if we couldn't
                    wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                }
                
                // Then start constructing the string that will be used to display all of the item's details
                strTooltipText = pEquipment->GetName();

                // If this item is equipped then we want to show that in the details string
                if (pEquipment->IsEquipped()) {
                    strTooltipText += " (E)";
                    bIsEquipped = true; // (And we'll need to remember that it's equipped later on)
                }

                // Add the rest of the item's details to the string
                strTooltipText += "\n\n" + pEquipment->GetDescription() + "\n\nValue: " + std::to_string(pEquipment->GetValue()) + "\nSlot: " + pEquipment->GetEquipmentSlotString();
            }
            else { // If for some reason this item isn't Consumable OR Equipment
                strTooltipText = pItem->GetName() + "\n\n" + pItem->GetDescription(); // We only show the name and the description
            }
            
            // We're also going to store a string representation of the slot index that we're on
            // so that we can create unique tooltips for each slot later
            std::string strIndex = std::to_string(k);

            // Now we can start making the actual buttons
            if (ImGui::Button("Item", ImVec2(50, 50))) {
            }
            
            // When we hover over an inventory slot
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                // We display the details string that we constructed earlier
                ImGui::BeginTooltip();
                ImGui::Text(strTooltipText.c_str());
                ImGui::EndTooltip();
            }

            // When we click on an inventory slot
            if (ImGui::IsItemClicked()) {
                // We open a little pop-up menu
                ImGui::OpenPopup(strIndex.c_str());
            }
            
            // The pop-up menu has different buttons based on what the item is and what "state" it's in
            if (ImGui::BeginPopup(strIndex.c_str())) {
                if (pItem->GetID() == CONSUMABLE) { // If the item is Consumable
                    // We need to be able to "use" it
                    if (ImGui::Button("Use")) {
                        this->UseItem(pItem, k);
                        ImGui::CloseCurrentPopup();
                    }
                }
                else if (pItem->GetID() == EQUIPMENT) { // If the item is a piece of Equipment
                    if (!bIsEquipped) { // We need to know if it is equipped
                        // If it isn't, we need to be able to put it on
                        if (ImGui::Button("Equip")) {
                            this->EquipItem(pItem, k);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    else {
                        // And if it IS equipped, we need to be able to take it off
                        if (ImGui::Button("Unequip")) {
                            this->UnequipItem(pItem, k);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                // We can discard any item we like
                if (ImGui::Button("Discard")) {
                    // If we're discarding an equipment item
                    if (pItem->GetID() == EQUIPMENT) {
                        // We need to make sure it's unequipped first
                        this->UnequipItem(pItem, k);
                    }
                    
                    this->RemoveItem(k);
                    // If we're discarding something, then it's *probably* safe to delete it, too
                    delete(pItem);
                    ImGui::CloseCurrentPopup();
                }

                // And we can close the pop-up menu whenever we like
                if (ImGui::Button("Close")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        else { // Otherwise, this is an empty inventory slot
            if(ImGui::Button("##", ImVec2(50, 50))) {
                // So nothing needs to happen!
            }
        }

        // If we've drawn the maximum number of slots per row
        if (counter == m_iMaxPerRow - 1) {
            // Reset the counter
            counter = 0;
        }
        else {
            // Otherwise, this slot needs to be drawn on the same line as the last one
            ImGui::SameLine();
            counter++;
        }
    }
    // End of window
    ImGui::End();
}

void InventoryComponent::UseItem(ItemBase* p_pItem, int p_iItemIndex) {
    ConsumableItem* pConsumable = static_cast<ConsumableItem*>(p_pItem);
    if (pConsumable) {
        // If this is the last use the item has left
        if (pConsumable->GetNumUses() == 1) {
            // Then we need to take it out of the inventory
            this->RemoveItem(p_iItemIndex);
        }

        // Then we can use it!
        pConsumable->UseItem();
    }
}

void InventoryComponent::EquipItem(ItemBase* p_pItem, int p_iItemIndex) {
    EquipmentItem* pEquipment = static_cast<EquipmentItem*>(p_pItem);
    if (pEquipment) {
        // If we're trying to equip something that we're already wearing
        if (pEquipment->IsEquipped()) {
            return; // Just return
        }

        // Figure out which slot this item equips into
        // Then, if there is something in that slot already, unequip it and equip this one
        switch (pEquipment->GetEquipmentSlot()) {
            case WEAPON:
            break;

            case HEAD:
            break;

            case BODY:
            break;

            case ARMS:
            break;

            case LEGS:
            break;

            case FEET:
            break;

            case GLOVES:
            break;

            case ACCESSORY:
            break;
        }

        pEquipment->SetEquipped(true);
    }
}

void InventoryComponent::UnequipItem(ItemBase* p_pItem, int p_iItemIndex) {
    EquipmentItem* pEquipment = static_cast<EquipmentItem*>(p_pItem);
    if (pEquipment) {
        // If for some reason we're trying to unequip something we don't have equipped
        if (!pEquipment->IsEquipped()) {
            return; // Just return
        }

        // Figure out which slot this item equips into so that we can keep track
        // of what slot we're unequipping from
        switch (pEquipment->GetEquipmentSlot()) {
            case WEAPON:
            break;

            case HEAD:
            break;

            case BODY:
            break;

            case ARMS:
            break;

            case LEGS:
            break;

            case FEET:
            break;

            case GLOVES:
            break;

            case ACCESSORY:
            break;
        }

        // Then unequip the item
        pEquipment->SetEquipped(false);
    }
}