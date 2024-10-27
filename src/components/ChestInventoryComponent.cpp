#include "ChestInventoryComponent.h"

ChestInventoryComponent::~ChestInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister the chest's listeners
    wolf::EventManager::RemoveListener<SendItemToChestEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleAddToChestEvent>(*this);
    wolf::EventManager::RemoveListener<RemoveFromChestEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleRemoveFromChestEvent>(*this);
    wolf::EventManager::RemoveListener<OpenInventoryEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<CloseInventoryEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleCloseInventoryEvent>(*this);
}

void ChestInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        float fNumRows = m_vvpContents.size() / m_iMaxPerRow;
        float fOffset = 18.25f;

        // For some silly reason, if the inventory can be shown on
        // a single row the inventory padding is a bit too small
        if (fNumRows == 1) {
            // So we add a little bit extra
            fNumRows += 0.4f;
        }
        else if (fNumRows == 2) {
            fNumRows += 0.2f;
        }
        
        // We run into a similar issue when we're only showing one item
        // on the X axis, so we add an extra offset to accomodate that
        if (m_iMaxPerRow == 1) {
            fOffset += 6.0f;
        }

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos({800, 200});
        ImGui::SetNextWindowSize({(m_v2TexFrameSize.x + fOffset) * m_iMaxPerRow, (m_v2TexFrameSize.y + 25) * fNumRows});
        ImGui::Begin("\t~ Chest ~", &m_bIsOpen, flags);

        // If we've closed the window using the ImGui button
        if (!m_bIsOpen) {
            // We need to call the actual close method
            this->Close();
        }

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
                    strTooltipText = pItem->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")" + "\n\n" + pItem->GetDescription(); // We only show the name and the description
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
                    if (ImGui::Button("Take")) {
                        SendItemToPlayer(k);
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
}

void ChestInventoryComponent::Open() {
    m_bIsOpen = true;

    // Let anyone interested know which specific chest was opened
    wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
}

void ChestInventoryComponent::Close() {
    m_bIsOpen = false;

    // Let anyone interested know which specific chest was closed
    wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
}

void ChestInventoryComponent::ToggleOpen() {
    m_bIsOpen = !m_bIsOpen;

    if (m_bIsOpen) {
        wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
    }
    else {
        wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
    }
}

void ChestInventoryComponent::SendItemToPlayer(int p_iItemIndex) {
    // Retrieve the item from the inventory and send it to the player via an event.
    wolf::EventManager::TriggerEvent(SendItemToPlayerInventoryEvent(m_enType, m_iIdNum, this->GetItem(p_iItemIndex), p_iItemIndex));
    
    // !-- Note that we also send along the index that this specific chest was storing the item at to make sure
    // that when we the player tells us to remove the item from the chest later we can remove the exact item that
    // we sent rather than the first instance of it (in case we have multiple items with the same name) --!
}

void ChestInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this chest is open
    if (m_bIsOpen) {
        // And a different chest is opening
        if (p_event.enType == CHEST_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
        }
    }
}

void ChestInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If the player just closed their inventory
    if (p_event.enType == PLAYER_INVENTORY) {
        // And this chest is open
        if (m_bIsOpen) {
            // Close it
            m_bIsOpen = false;
        }
    }
}

void ChestInventoryComponent::HandleAddToChestEvent(const SendItemToChestEvent& p_event) {
    // If the player is trying to add an item to this specific chest
    if (p_event.iChestIdNum == m_iIdNum) {
        // And we have the space to hold it
        if (this->AddItem(p_event.pItem)) {
            // Then we need to let the player know that they can remove the item from their inventory
            wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(p_event.pItem->GetName(), p_event.iPlayerInventoryIndex));
        }
    }
}

void ChestInventoryComponent::HandleRemoveFromChestEvent(const RemoveFromChestEvent& p_event) {
    // If someone has taken an item out of this specific chest
    if (p_event.iChestIdNum == m_iIdNum) {
        // Then we check if they knew what index of the chest's inventory the item was stored at
        if (p_event.iChestInventoryIndex != -1) {
            // And if they did, we remove whatever item is there
            this->RemoveItem(p_event.iChestInventoryIndex);
        }
        else {
            // Otherwise, we remove the first instance of the item that we can find by using its name
            this->RemoveItem(p_event.strItemName);
        }
    }
}