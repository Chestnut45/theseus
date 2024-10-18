#include "InventoryComponent.h"
#include "W_Logging.h"

InventoryComponent::InventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize) : m_iSize(p_iSize), m_iMaxPerRow(p_iSlotsPerRow) {
    // Reserve the amount of space we've been asked for
    m_vvpContents.reserve(p_iSize);

    // And fill it up with empty stacks
    for (int i = 0; i < p_iSize; i++) {
        std::stack<ItemBase*> stack;
        m_vvpContents.push_back(stack);
    }

    wolf::Texture* pNewTexture = wolf::TextureManager::CreateTexture(p_strTexture);
    if (pNewTexture) {
        if ((pNewTexture->GetWidth() * pNewTexture->GetHeight()) % (int)(p_v2TexFrameSize.x * p_v2TexFrameSize.y) != 0) {
            // If the new texture doesn't match the frame size then we delete it and leave the current texture unchanged
            wolf::TextureManager::DestroyTexture(pNewTexture);
            wolf::Error("Incorrectly sized texture file \"", p_strTexture, "\" passed to InventoryComponent.");
        }
        // Then we need to know how many frames are in the texture
        int iNumFramesX = pNewTexture->GetWidth() / p_v2TexFrameSize.x;
        int iNumFramesY = pNewTexture->GetHeight() / p_v2TexFrameSize.y;

        int iWidth = iNumFramesX + 1;
        int iHeight = iNumFramesY + 1;

        // We can use those values to create UV coordinates by treating them
        // as points between 0 and 1 on the X and Y axes

        // So we make a place to store them
        ImVec2 av2WorkingUVCoords[iWidth * iHeight];

        // Figure out how much we'll be incrementing each X and Y by
        float fIncX = 1.0f / iNumFramesX;
        float fIncY = 1.0f / iNumFramesY;

        // And start calculating them
        for (int i = 0; i <= iNumFramesY; i++) {
            // A V coordinate would be calculated as:
            float fVCord = i * fIncY;

            for (int j = 0; j <= iNumFramesX; j++) {
                // And a U coordinate would be calculated the same way, but with x
                float fUCord = j * fIncX;

                // Once we have a UV coordinate set we store it for later
                av2WorkingUVCoords[(i * iWidth) + j] = ImVec2(fUCord, fVCord);

            }
        }

        // Because frames are numbered 1-n but vectors are index 0-n, we need an offset
        // frame coordinate set that occupies the first index.
        ImGuiUVSet* pOffsetCoord = new ImGuiUVSet(ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
        m_vv2TextureCoords.push_back(pOffsetCoord);

        // Now that we have all our UV coordinates, we're going to assign them to frames
        for (int p = 0; p <= iNumFramesY - 1; p++) {
            for (int q = 0; q <= iNumFramesX - 1; q++) {
                int iOriginPoint = (p * iWidth) + q;

                // First we find the four UV coordinates that will be used to render this frame
                // and store them in a struct that holds four glm::vec2s
                ImGuiUVSet* pTexFrameCords = new ImGuiUVSet(av2WorkingUVCoords[iOriginPoint], av2WorkingUVCoords[iOriginPoint + iWidth + 1]);

                // Then we store 'em
                m_vv2TextureCoords.push_back(pTexFrameCords);
            }
        }

        pNewTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        m_pTexture = pNewTexture;
        m_v2TexFrameSize = ImVec2(p_v2TexFrameSize.x, p_v2TexFrameSize.y);
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
            delete pNextItem;
            it->pop();
        }
    }
}

void InventoryComponent::ShowInventoryGUI() {
    // You can't resize the inventory but you can move it around!
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    // By default, the inventory appears close to the middle of the screen
    ImGui::SetNextWindowPos({500, 200});
    ImGui::SetNextWindowSize({(m_v2TexFrameSize.x + 20) * m_iMaxPerRow, (m_v2TexFrameSize.y + 20) * m_iMaxPerRow});
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
                ConsumableItem* pConsumable = dynamic_cast<ConsumableItem*>(pItem);
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
                EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
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
            if (ImGui::ImageButton("Filled Slot", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[pItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2TextureCoords[pItem->GetTextureFrameIndex()]->m_v2BotRight)) {
            }
            
            // When we hover over an inventory slot
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                // We display the details string that we constructed earlier
                ImGui::BeginTooltip();
                ImGui::Text("%s", strTooltipText.c_str());
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
                            this->UnequipItem(pItem);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                // We can discard any item we like
                if (ImGui::Button("Discard")) {
                    this->DiscardItem(k);
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
           if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[NONE]->m_v2TopLeft, m_vv2TextureCoords[NONE]->m_v2BotRight)) {

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
    ConsumableItem* pConsumable = dynamic_cast<ConsumableItem*>(p_pItem);
    if (pConsumable) {
        // If this is the last use the item has left
        if (pConsumable->GetNumUses() == 1) {
            // Then we need to take it out of the inventory
            this->RemoveItem(p_iItemIndex);
        }

        // Then we can use it!
        pConsumable->Use();
    }
}

void InventoryComponent::EquipItem(ItemBase* p_pItem, int p_iItemIndex) {
    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(p_pItem);
    if (pEquipment) {
        // If we're trying to equip something that we're already wearing
        if (pEquipment->IsEquipped()) {
            return; // Just return
        }

        // First we need to figure out if there is already something equipped in the slot
        // that the item we're trying to equip corresponds to
        int iPrevItemIndex = m_iEquipmentSlots[pEquipment->GetEquipmentSlot()];

        // If there is, the index will be a positive integer (or zero)
        if (iPrevItemIndex >= 0) {
            // So we need to retrieve and then unequip the item at that index
            this->UnequipItem(this->GetItem(iPrevItemIndex));
        }

        // Then we can store the index of the newly equipped item
        m_iEquipmentSlots[pEquipment->GetEquipmentSlot()] = p_iItemIndex;

        // And let the item know it has been equipped
        pEquipment->SetEquipped(true);
    }
}

void InventoryComponent::UnequipItem(ItemBase* p_pItem) {
    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(p_pItem);
    if (pEquipment) {
        // If for some reason we're trying to unequip something we don't have equipped
        if (!pEquipment->IsEquipped()) {
            return; // Just return
        }

        // Figure out which slot this item equips into and "empty" that slot by setting it to an invalid index
        m_iEquipmentSlots[pEquipment->GetEquipmentSlot()] = -1;

        // Then let the item know it's been unequipped
        pEquipment->SetEquipped(false);
    }
}

// This is a wrapper for GetItem that retrieves whichever item in the inventory is equipped
// in a given equipment slot, or nullptr if there is no item currently equipped in that slot
ItemBase* InventoryComponent::GetEquippedItem(EquipmentSlot p_enSlot) {
    return GetItem(m_iEquipmentSlots[p_enSlot]);
}

void InventoryComponent::DiscardItem(int p_iItemIndex) {
    // First find the item we want to discard
    ItemBase* pItem = m_vvpContents[p_iItemIndex].top();

    // If it is an equipment item
    if (pItem->GetID() == EQUIPMENT) {
        // We need to make sure it is unequipped
        this->UnequipItem(pItem);
    }

    // Then we can remove it from the inventory
    this->RemoveItem(p_iItemIndex);

    // And delete it!
    delete pItem;
}