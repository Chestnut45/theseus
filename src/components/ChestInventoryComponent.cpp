#include "ChestInventoryComponent.h"

ChestInventoryComponent::~ChestInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister the chest's listeners
    wolf::EventManager::RemoveListener<OpenInventoryEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleOpenInventoryEvent>(*this);
}

bool ChestInventoryComponent::FillChestFromFile(const std::string& p_strFilePath) {
    return false;
}

void ChestInventoryComponent::OpenChest() {
    m_bIsOpen = true;
    wolf::EventManager::EnqueueEvent(OpenInventoryEvent(m_enType, this));
}

void ChestInventoryComponent::CloseChest() {
    m_bIsOpen = false;
    wolf::EventManager::EnqueueEvent(CloseInventoryEvent(m_enType, this));
}

void ChestInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos({800, 200});
        ImGui::SetNextWindowSize({(m_v2TexFrameSize.x + 18.25f) * m_iMaxPerRow, (m_v2TexFrameSize.y + 22) * m_iMaxPerRow});
        ImGui::Begin("\t~ Chest ~", nullptr, flags);

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

void ChestInventoryComponent::SendItemToPlayer(int p_iItemIndex) {
    // Retrieve the item from the inventory and send it to the player via an event
    wolf::EventManager::EnqueueEvent(this->GetItem(p_iItemIndex));

    // Then take the item out of the chest
    this->RemoveItem(p_iItemIndex);
}

void ChestInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this chest is open
    if (m_bIsOpen) {
        // And a different chest is opening
        if (p_event.enType == CHEST_INVENTORY && p_event.pInventory != this) {
            // Close this one
            m_bIsOpen = false;
        }
    }
}