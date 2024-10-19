#include "PlayerInventoryComponent.h"

PlayerInventoryComponent::~PlayerInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister the chest's listeners
    wolf::EventManager::RemoveListener<OpenInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<AddToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<DeleteFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDeleteFromPlayerInventoryEvent>(*this);
}

void PlayerInventoryComponent::ShowInventoryGUI() {
    // You can't resize the inventory or move it
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    // By default, the inventory appears close to the middle of the screen
    ImGui::SetNextWindowPos({500, 200});
    ImGui::SetNextWindowSize({(m_v2TexFrameSize.x + 18.25f) * m_iMaxPerRow, (m_v2TexFrameSize.y + 22) * m_iMaxPerRow});
    ImGui::Begin("\t~ Inventory ~", nullptr, flags);

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

    ImGui::Text("Gold: %d", m_iGold);

    // End of window
    ImGui::End();
}

void PlayerInventoryComponent::UseItem(ItemBase* p_pItem, int p_iItemIndex) {
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

void PlayerInventoryComponent::EquipItem(ItemBase* p_pItem, int p_iItemIndex) {
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

void PlayerInventoryComponent::UnequipItem(ItemBase* p_pItem) {
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
ItemBase* PlayerInventoryComponent::GetEquippedItem(EquipmentSlot p_enSlot) {
    return GetItem(m_iEquipmentSlots[p_enSlot]);
}

void PlayerInventoryComponent::DiscardItem(int p_iItemIndex) {
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

void PlayerInventoryComponent::AddGold(int p_iAmt) {
    m_iGold += p_iAmt;

    if (m_iGold > MAX_GOLD) {
        m_iGold = MAX_GOLD;
    }
}

bool PlayerInventoryComponent::TakeGold(int p_iAmt) {
    if (m_iGold - p_iAmt < 0) {
        return false;
    }
    
    m_iGold -= p_iAmt;
    return true;
}

void PlayerInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    if (p_event.enType != PLAYER_INVENTORY) {
        
    }
}

void PlayerInventoryComponent::HandleAddToPlayerInventoryEvent(const AddToPlayerInventoryEvent& p_event) {
    this->AddItem(p_event.pItem);
}

void PlayerInventoryComponent::HandleDeleteFromPlayerInventoryEvent(const DeleteFromPlayerInventoryEvent& p_event) {
    ItemBase* pItem = this->GetItem(p_event.strItemName);
    this->RemoveItem(p_event.strItemName);
    delete(pItem);
}