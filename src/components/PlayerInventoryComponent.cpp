#include "PlayerInventoryComponent.h"

#include "../inventory/ArmourItem.h"

PlayerInventoryComponent::~PlayerInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister the chest's listeners
    wolf::EventManager::RemoveListener<OpenInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<CloseInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleCloseInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<SellItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleSellItemToPlayerEvent>(*this);
    wolf::EventManager::RemoveListener<PickupDroppedItemEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandlePickupDroppedItemEvent>(*this);
    wolf::EventManager::RemoveListener<DispenseItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDispenseItemToPlayerEvent>(*this);
    wolf::EventManager::RemoveListener<SendItemToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<RemoveFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerInventoryEvent>(*this);
}

void PlayerInventoryComponent::ShowToggleButtonGUI() {
    ImGuiStyle* pStyle = &ImGui::GetStyle();
    pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // You can't resize the inventory or move it
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

    // By default, the inventory appears close to the middle of the screen
    ImGui::SetNextWindowPos(ImVec2(100, 500));
    ImGui::SetNextWindowSize({0,0});
    ImGui::Begin("InventoryToggleButton", nullptr, flags);

    pStyle->Colors[ImGuiCol_Button] = ImVec4(0, 0, 0, 0);
    pStyle->Colors[ImGuiCol_ButtonActive] = ImVec4(0, 0, 0, 0);
    pStyle->Colors[ImGuiCol_ButtonHovered] = ImVec4(0, 0, 0, 0);

    pStyle->WindowRounding = 32.0f;
    pStyle->FrameRounding = 50.0f;

    if (ImGui::ImageButton("InventoryToggleButton", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[17]->m_v2TopLeft, m_vv2TextureCoords[17]->m_v2BotRight)) {
        this->ToggleOpen();
        pStyle->Colors[ImGuiCol_WindowBg] = ImVec4(0, 1, 0, 1);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        pStyle->Colors[ImGuiCol_WindowBg] = ImVec4(1, 0, 0, 1);
    }
    else {
        pStyle->Colors[ImGuiCol_WindowBg] = ImVec4(1, 0, 1, 1);
    }

    ImGui::End();
}

void PlayerInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(m_v2DrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin("~ Inventory ~", &m_bIsOpen, flags);

        // If we closed the inventory
        if (!m_bIsOpen) {
            // Let anyone interested know
            wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
        }

        // This counter lets us control how many items are drawn in a row
        int counter = 0;

        // We need to draw m_iSize number of slots
        for (int k = 0; k < m_iSize; k++) {
            // If there is an item (or stack of items as it were) in this slot
            if (!m_vvpContents[k].empty()) {
                // We grab a reference to the top item and create a variable to hold the item's details
                ItemBase* pItem = m_vvpContents[k].top();
                std::string strTooltipText = pItem->GetToolTipText() + "\n\nValue: " + std::to_string(pItem->GetValue());
                std::string strTooltipName;

                // There are different rules for drawing Consumables and Equipment Items so we need to figure out
                // what this particular item is before we go any further

                // If this is a consumable item
                if (pItem->GetID() == CONSUMABLE) {
                    // Try to cast it
                    ConsumableItem* pConsumable = dynamic_cast<ConsumableItem*>(pItem);
                    if (!pConsumable) {
                        // And throw an error if we couldn't
                        wolf::Error("Failed to cast ItemBase to ConsumableItem!\n");
                    }

                    // Then construct the string that will be used to display all of the item's name
                    strTooltipName = pConsumable->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")";
                }
                else if (pItem->GetID() == EQUIPMENT) { // If this is an equipment item
                    // Try to cast it
                    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
                    if (!pEquipment) {
                        // And throw an error if we couldn't
                        wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                    }
                    
                    // Then start constructing the string that will be used to display the item's name
                    strTooltipName = pEquipment->GetName();
                    
                }
                else { // If for some reason this item isn't Consumable OR Equipment
                    strTooltipName = pItem->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")";
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

                    // Display the item name in the color that corresponds to its rarity level
                    RGBIntColor nameColor = RarityColors[pItem->GetRarity()];
                    ImGui::TextColored(ImColor(nameColor.r, nameColor.g, nameColor.b), "%s", strTooltipName.c_str());

                    // Display the item's description
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + TOOLTIP_WRAP_POS);
                    ImGui::TextWrapped("%s", strTooltipText.c_str());
                    ImGui::PopTextWrapPos();

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
                        // We need to be able to equip it
                        if (ImGui::Button("Equip")) {
                            this->EquipItem(pItem, k);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // If we currently have a chest open
                    if (m_iOpenChestIdNum != -1) {
                        // Then we need to be able to move items into it
                        if (ImGui::Button("Store")) {
                            // We move items by sending an event to the open chest
                            // !-- Note that we send the index that we're storing the item at in the player's inventory so that
                            // when the chest sends a return message telling us to remove the item from the player's inventory,
                            // we can make remove the specific item we sent rather than the first instance of it in our inventory --!
                            wolf::EventManager::TriggerEvent(SendItemToChestEvent(m_iOpenChestIdNum, this->GetItem(k), k));
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // If we are currently talking to a merchant
                    if (m_iOpenMerchantIdNum != -1) {
                        // The we need to be able to sell items to them
                        if (ImGui::Button("Sell")) {
                            // We sell an item by sending an event to the merchant we're talking to
                            // !-- Note that we send along the index that we're storing the item at in the player's inventory so that
                            // when the merchant sends a return message telling us that the item has been purchased we can remove the
                            // specific item that we sent rather than the first instance of it in our inventory --!
                            wolf::EventManager::TriggerEvent(SellItemToMerchantEvent(m_iOpenMerchantIdNum, this->GetItem(k), k));
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    if (ImGui::Button("Drop")) {
                        wolf::Transform2D* pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
                        if (pTransform) {
                            ItemDropCreator::Instance()->CreateItemDropFromExistingItem(pItem, pTransform->GetGlobalPosition(), 5.0f);
                        }
                        this->RemoveItem(k);
                        ImGui::CloseCurrentPopup();
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
                if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2TextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {

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

        // Reset the counter because we're moving on to a new section of the inventory
        counter = 0;

        // Draw the equipped items under a separator line
        ImGui::SeparatorText("Equipped");
        for (int t = 0; t < END_OF_EQUIPMENT; t++) {
            // Get the item equipped in this slot
            EquipmentItem* pEquipItem = m_pEquipment[t];

            // If there is an item equipped in the slot
            if (pEquipItem) {
                // Get the name and item details
                std::string strEquipName = pEquipItem->GetName();
                std::string strEquipTooltip = pEquipItem->GetToolTipText();

                // Draw the inventory slot
                if (ImGui::ImageButton("Equipment Slot", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[pEquipItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2TextureCoords[pEquipItem->GetTextureFrameIndex()]->m_v2BotRight)) {
                }

                // Same as a regular item, when we hover over an equipment slot we display the item's details in a tooltip
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    // We display the details string that we constructed earlier
                    ImGui::BeginTooltip();

                    // Display the item name in the color that corresponds to its rarity level
                    RGBIntColor equipNameColor = RarityColors[pEquipItem->GetRarity()];
                    ImGui::TextColored(ImColor(equipNameColor.r, equipNameColor.g, equipNameColor.b), "%s", strEquipName.c_str());

                    // Display the item's description
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + TOOLTIP_WRAP_POS);
                    ImGui::TextWrapped("%s", strEquipTooltip.c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::EndTooltip();
                }

                // We need a way for ImGui to differientiate between equipment slots so we make an id string
                std::string strPopUpID = "E:" + std::to_string(t);

                // When we click on an equipped item
                if (ImGui::IsItemClicked()) {
                    // We open a little pop-up menu
                    ImGui::OpenPopup(strPopUpID.c_str());
                }
                
                // The pop-up menu has different buttons based on what "state" the game is in
                if (ImGui::BeginPopup(strPopUpID.c_str())) {
                    // We can unequip items
                    if (ImGui::Button("Unequip")) {
                        this->UnequipItem(pEquipItem);
                        ImGui::CloseCurrentPopup();
                    }

                    // If we currently have a chest open
                    if (m_iOpenChestIdNum != -1) {
                        // Then we need to be able to move items into it
                        if (ImGui::Button("Store")) {
                            // We move items by sending an event to the open chest
                            // !-- Note that we send along the "index" or slot that the item is equipped at and a flag
                            // indicating that the item is/was equipped--!
                            wolf::EventManager::TriggerEvent(SendItemToChestEvent(m_iOpenChestIdNum, pEquipItem, t, true));
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // If we are currently talking to a merchant
                    if (m_iOpenMerchantIdNum != -1) {
                        // The we need to be able to sell items to them
                        if (ImGui::Button("Sell")) {
                            // We sell an item by sending an event to the merchant we're talking to
                            // !-- Note that we send along the "index" or slot that the item is equipped at and a flag
                            // indicating that the item is/was equipped--!
                            wolf::EventManager::TriggerEvent(SellItemToMerchantEvent(m_iOpenMerchantIdNum, pEquipItem, t, true));
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // We can drop equipment
                    if (ImGui::Button("Drop")) {
                        wolf::Transform2D* pTransform = this->GetGameObject()->GetComponent<wolf::Transform2D>();
                        if (pTransform) {
                            ItemDropCreator::Instance()->CreateItemDropFromExistingItem(pEquipItem, pTransform->GetGlobalPosition(), 5.0f);
                        }
                        this->RemoveEquippedItem(pEquipItem->GetEquipmentSlot());
                        ImGui::CloseCurrentPopup();
                    }

                    // We can discard equipment
                    if (ImGui::Button("Discard")) {
                        this->DiscardEquipment(pEquipItem->GetEquipmentSlot());
                        ImGui::CloseCurrentPopup();
                    }

                    // And we can close the pop-up menu whenever we like
                    if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            else {
                // Otherwise, this slot is empty
                if (ImGui::ImageButton("Empty Equipment Slot", (void*)(intptr_t)m_pTexture->GetID(), m_v2TexFrameSize, m_vv2TextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2TextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {
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

        // Iterate through the schematic counters
        ImGui::Text("Schematics:");
        for (int p = 0; p < END_OF_RARITIES; p++) {
            // Retrieve the color associated with this rarity
            RGBIntColor color = RarityColors[p];

            // And draw the counter's value in that color
            ImGui::SameLine();
            ImGui::TextColored(ImColor(color.r, color.g, color.b), "%d", m_iSchematics[p]);
        }

        // Draw the player's gold value
        ImGui::Text("Gold:");
        ImGui::SameLine();
        ImGui::TextColored(ImColor(255, 215, 0), "%d", m_iGold); // in gold (ha)

        ImVec2 v2MainWindowSize = ImGui::GetWindowSize();

        // End of window
        ImGui::End();

        // If our pockets are full and we tried to add an item to 'em
        if (m_bShowFullInventoryPrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({m_v2DrawPos.x - v2MainWindowSize.x / 2.0f, m_v2DrawPos.y + v2MainWindowSize.y / 2.0f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Inventory Is Full Prompt", nullptr, flags);

            // Show a message asking the player if they are okay with selling the item for less than its value
            ImGui::Text("Your inventory is full.");
            ImGui::NewLine();
            ImGui::Text("\t   ");
            ImGui::SameLine();
            
            // If they are
            if (ImGui::Button("Close")) {
                // Close this prompt and process the sale
                m_bShowFullInventoryPrompt = false;
            }
            ImGui::End();
        }

        // If we tried to sell something to a merchant but they can't afford it
        if (m_bShowTooExpensivePrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({m_v2DrawPos.x - v2MainWindowSize.x / 4.0f, m_v2DrawPos.y + v2MainWindowSize.y / 4.0f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Too Expensive Prompt", nullptr, flags);

            // Show a message asking the player if they are okay with selling the item for less than its value
            ImGui::Text("You don't have enough gold to buy that.");
            ImGui::NewLine();
            ImGui::Text("\t\t\t\t");
            ImGui::SameLine();
            
            // If they are
            if (ImGui::Button("Close")) {
                // Close this prompt and process the sale
                m_bShowTooExpensivePrompt = false;
            }
            ImGui::End();
        }

        // If we tried to get an item that we don't have a schematic for from a dispensary
        if (m_bShowMissingSchematicPrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({m_v2DrawPos.x - v2MainWindowSize.x / 4.0f, m_v2DrawPos.y + v2MainWindowSize.y / 4.0f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Missing Schematic Prompt", nullptr, flags);

            // Show a message asking the player if they are okay with selling the item for less than its value
            ImGui::Text("You don't have a schematic to trade for that.");
            ImGui::NewLine();
            ImGui::Text("\t\t\t\t");
            ImGui::SameLine();
            
            // If they are
            if (ImGui::Button("Close")) {
                // Close this prompt and process the sale
                m_bShowMissingSchematicPrompt = false;
            }
            ImGui::End();
        }
    }
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

        // When we equip an item we take it out of the "main" inventory and
        // store it in a "sub-inventory" that is just for equipped items
        this->RemoveItem(p_iItemIndex);

        // Before we can do that, though, we need to check if there is already
        // something equipped in the slot that the item equips into
        EquipmentItem* pPrevItem = m_pEquipment[pEquipment->GetEquipmentSlot()];

        // If there is,
        if (pPrevItem) {
            // We need to unequip that item
            pPrevItem->SetEquipped(false);

            // And add it back into the player's inventory
            this->AddItem(pPrevItem);
        }

        // Then we can store the new item in corresponding equipment slot
        m_pEquipment[pEquipment->GetEquipmentSlot()] = pEquipment;

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

        // Because equipped items are stored in a sub-inventory,
        // we can only unequip something if we have space to hold it
        if (this->AddItem(m_pEquipment[pEquipment->GetEquipmentSlot()])) {
            // If the item was added back into the main inventory successfully, we "empty" the sub-inventory slot
            m_pEquipment[pEquipment->GetEquipmentSlot()] = nullptr;

            // And let the item know it's been unequipped
            pEquipment->SetEquipped(false);
        }
        else {
            // Otherwise, we want to let the player know that their inventory is too full to unequip the item
            m_bShowFullInventoryPrompt = true;
        }
    }
}

// This is a wrapper for GetItem that retrieves whichever item in the inventory is equipped
// in a given equipment slot, or nullptr if there is no item currently equipped in that slot
ItemBase* PlayerInventoryComponent::GetEquippedItem(EquipmentSlot p_enSlot) {
    return m_pEquipment[p_enSlot];
}

// This method DOES NOT delete the equipped item.
// If you want to delete the item, retrieve it first using GetEquippedItem()
void PlayerInventoryComponent::RemoveEquippedItem(EquipmentSlot p_enSlot) {
    // If there is an item equipped in this slot
    if (m_pEquipment[p_enSlot]) {
        // Unequip it and "empty" the slot by setting it to nullptr
        m_pEquipment[p_enSlot]->SetEquipped(false);
        m_pEquipment[p_enSlot] = nullptr;
    }
}

void PlayerInventoryComponent::DiscardItem(int p_iItemIndex) {
    // First find the item we want to discard
    ItemBase* pItem = m_vvpContents[p_iItemIndex].top();

    // Then remove it from the inventory
    this->RemoveItem(p_iItemIndex);

    // And delete it!
    delete pItem;
}

void PlayerInventoryComponent::DiscardEquipment(EquipmentSlot p_enSlot) {
    // Get the item that we want to discard
    EquipmentItem* pEquipment = m_pEquipment[p_enSlot];

    // Unequip it
    pEquipment->SetEquipped(false);

    // Empty the slot it was just in
    m_pEquipment[p_enSlot] = nullptr;

    // And delete it!
    delete pEquipment;
}

void PlayerInventoryComponent::AddGold(int p_iAmt) {
    // Add some more gold to our "wallet"
    m_iGold += p_iAmt;

    // If that sends us over the limit
    if (m_iGold > MAX_GOLD) {
        // Pretend that it didn't.
        m_iGold = MAX_GOLD;
    }
}

bool PlayerInventoryComponent::TakeGold(int p_iAmt) {
    // If taking away the given amount of gold would send us into the negative
    if ((m_iGold - p_iAmt) < 0) {
        // Then we return false and don't deplete the gold
        return false;
    }
    
    // Otherwise, we just take away the given amount and return true
    m_iGold -= p_iAmt;
    return true;
}

void PlayerInventoryComponent::AddSchematic(Rarity p_enRarity) {
    // Add the schematic to the counter for that particular rarity level
    m_iSchematics[p_enRarity] += 1;

    // If that sends us over the limit
    if (m_iSchematics[p_enRarity] > MAX_SCHEMATICS_PER_RARITY) {
        // Pretend it didn't.
        m_iSchematics[p_enRarity] = MAX_SCHEMATICS_PER_RARITY;
    }
}

bool PlayerInventoryComponent::TakeSchematic(Rarity p_enRarity) {
    // If taking away a schematic would send the rarity counter into the negative
    if ((m_iSchematics[p_enRarity] - 1) < 0) {
        // Then return false and don't take it away
        return false;
    }

    // Otherwise, take the schematic and return true
    m_iSchematics[p_enRarity] -= 1;
    return true;
}

int PlayerInventoryComponent::GetNumSchematics() {
    int iCount = 0;

    // Go through each of the rarity levels
    for (int k = 0; k < END_OF_RARITIES; k++) {
        iCount += m_iSchematics[k]; // And add the amount stored in each to the counter
    }

    // Then return the count
    return iCount;
}

void PlayerInventoryComponent::Close() {
    // Close the inventory
    m_bIsOpen = false;

    // Then let anyone interested know it happened
    wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
}

void PlayerInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If we have opened a chest (and it's not the chest we already have open)
    if (p_event.enType == CHEST_INVENTORY && p_event.iIdNum != m_iOpenChestIdNum) {
        // Then we need to keep track of its id number so we can move items between the two inventories
        m_iOpenChestIdNum = p_event.iIdNum;
    }
    else if (p_event.enType == MERCHANT_INVENTORY && p_event.iIdNum != m_iOpenChestIdNum) {
        // We do the same with merchant inventories
        m_iOpenMerchantIdNum = p_event.iIdNum;
    }

    // In any case, we want to open the player's inventory, too
    m_bIsOpen = true;
}

void PlayerInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If we have closed the chest that we are holding the id number for
    if (p_event.enType == CHEST_INVENTORY && p_event.iIdNum == m_iOpenChestIdNum) {
        // Then we can safely discard the id number because we're done moving items between the two inventories
        m_iOpenChestIdNum = -1;
    }
    else if (p_event.enType == MERCHANT_INVENTORY && p_event.iIdNum == m_iOpenMerchantIdNum) {
        // We do the same with merchant inventories
        m_iOpenMerchantIdNum = -1;
    }
}

void PlayerInventoryComponent::HandleSellItemToPlayerEvent(const SellItemToPlayerEvent& p_event) {
    // If we can afford this item
    if (this->TakeGold(p_event.iPrice)) {
        // If this is a schematic item
        if (p_event.pItem->GetID() == SCHEMATIC) {
            // It needs to be added to the schematic counter(s) rather than the inventory itself
            this->AddSchematic(p_event.pItem->GetRarity());

            // Then we need to let the merchant know we've processed our end of the sale so that they can process theirs
            wolf::EventManager::TriggerEvent(BoughtItemFromMerchantEvent(p_event.iMerchantIdNum, p_event.pItem->GetName(), p_event.iPrice, p_event.iMerchantInventoryIndex));
        }
        else if (this->AddItem(p_event.pItem)) { // If this ISN'T a schematic and we have enough room to store it
            // Then we need to let the merchant know we've processed our end of the sale so that they can process theirs
            wolf::EventManager::TriggerEvent(BoughtItemFromMerchantEvent(p_event.iMerchantIdNum, p_event.pItem->GetName(), p_event.iPrice, p_event.iMerchantInventoryIndex));
        }
        else { // If we didn't have room for the item but we did have enough money for it,
            // Then we need to add the money we "spent" back into the player's inventory
            this->AddGold(p_event.iPrice);

            // And let the player know we didn't have room for the item
            m_bShowFullInventoryPrompt = true;
        }
    }
    else {
        // If we can't afford the item we're trying to buy, we should let the player know
        m_bShowTooExpensivePrompt = true;
    }
}

void PlayerInventoryComponent::HandleAddToPlayerInventoryEvent(const SendItemToPlayerInventoryEvent& p_event) {
    // If this is a gold item
    if (p_event.pItem->GetID() == GOLD) {
        // Then we don't add it to our inventory, we just add the gold to our wallet
        this->AddGold(p_event.pItem->GetValue());

        // And let the chest know it no longer has the item
        wolf::EventManager::TriggerEvent(RemoveFromChestEvent(p_event.iSenderIdNum, p_event.pItem->GetName(), p_event.iSenderInventoryIndex));
    }
    else if (p_event.pItem->GetID() == SCHEMATIC) { // If this is a schematic item
        // Then we don't add it to our inventory either, we add it to our schematic counter(s)
        this->AddSchematic(p_event.pItem->GetRarity());

        // And let the chest know it no longer has the item
        wolf::EventManager::TriggerEvent(RemoveFromChestEvent(p_event.iSenderIdNum, p_event.pItem->GetName(), p_event.iSenderInventoryIndex));
    }
    else {
        // If we have space in our inventory for the item
        if (this->AddItem(p_event.pItem)) {
            // And we took the item out of a chest
            if (p_event.enSenderType == CHEST_INVENTORY) {
                // Then we need to let the chest know that it no longer has the item
                wolf::EventManager::TriggerEvent(RemoveFromChestEvent(p_event.iSenderIdNum, p_event.pItem->GetName(), p_event.iSenderInventoryIndex));
            }
        }
        else {
            // If we can't fit it in our inventory, then we should let the player know
            m_bShowFullInventoryPrompt = true;
        }
    }
}

void PlayerInventoryComponent::HandleRemoveFromPlayerInventoryEvent(const RemoveFromPlayerInventoryEvent& p_event) {
    // If the sender knew what index *this* inventory is storing the item at
    if (p_event.iIndex != -1) {
        // Then we can remove whatever item is at that specific index
        this->RemoveItem(p_event.iIndex);
    }
    else {
        // Otherwise, we remove the item by name (so we remove the first instance of it that we find)
        this->RemoveItem(p_event.strItemName);
    }

    // If we we're removing the item because we sold it to someone, then we'll want to add the amount we sold it for
    // to our wallet. (If we didn't sell the item this is technically a pointless function call)
    this->AddGold(p_event.iItemSoldFor);
}

void PlayerInventoryComponent::HandleRemoveFromPlayerEquipmentEvent(const RemoveFromPlayerEquipmentEvent& p_event) {
    // Call the remove method for equipped items
    this->RemoveEquippedItem(static_cast<EquipmentSlot>(p_event.iEquipSlot));

    // If we we're removing the item because we sold it to someone, then we'll want to add the amount we sold it for
    // to our wallet. (If we didn't sell the item this is technically a pointless function call)
    this->AddGold(p_event.iItemSoldFor);
}

void PlayerInventoryComponent::HandleDispenseItemToPlayerEvent(const DispenseItemToPlayerEvent& p_event) {
    // Check if we have a schematic with the same rarity level of the item we're receiving
    if (this->TakeSchematic(p_event.pItem->GetRarity())) {
        // If we do, try to add the item to our inventory
        if (!this->AddItem(p_event.pItem)) {
            // If the item doesn't fit in our inventory, we need to "unspend" the schematic we used to pay for it
            this->AddSchematic(p_event.pItem->GetRarity());

            // We also need to let the player know that they couldn't fit the item in their inventory
            m_bShowFullInventoryPrompt = true;
        }
    }
    else {
        // If we didn't have a matching schematic we need to let the player know
        m_bShowMissingSchematicPrompt = true;
    }
}

void PlayerInventoryComponent::HandlePickupDroppedItemEvent(const PickupDroppedItemEvent& p_event) {
    // If this is a gold item
    if (p_event.pItem->GetID() == GOLD) {
        // Then we don't add it to our inventory, we just add the gold to our wallet
        this->AddGold(p_event.pItem->GetValue());

        // And let the dropped item know it has been picked up
        wolf::EventManager::TriggerEvent(DestroyDroppedItemEvent(p_event.iDroppedItemId));
    }
    else if (p_event.pItem->GetID() == SCHEMATIC) { // If this is a schematic item
        // Then we don't add it to our inventory either, we add it to our schematic counter(s)
        this->AddSchematic(p_event.pItem->GetRarity());

        // And let the dropped item know it has been picked up
        wolf::EventManager::TriggerEvent(DestroyDroppedItemEvent(p_event.iDroppedItemId));
    }
    else {
        // If we have space in our inventory for the item
        if (this->AddItem(p_event.pItem)) {
            // Let the dropped item know it has been picked up
            wolf::EventManager::TriggerEvent(DestroyDroppedItemEvent(p_event.iDroppedItemId));
        }
    }
}