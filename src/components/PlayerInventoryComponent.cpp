//-----------------------------------------------------------------------------
// File:            PlayerInventoryComponent.cpp
// Original Author: Aurora Ryder
//
// A class representing the player's inventory
//-----------------------------------------------------------------------------

#include "PlayerInventoryComponent.h"

#include "../inventory/ArmourItem.h"

std::vector<ImGuiUVSet*> PlayerInventoryComponent::m_vv2ToggleTextureCoords;
const std::string PlayerInventoryComponent::m_strToggleTexturePath = "data/textures/InventoryToggleButton-Sheet.png";

const int PlayerInventoryComponent::TOG_BUTTON_CLOSED = 1;
const int PlayerInventoryComponent::TOG_BUTTON_CLOSED_HOVER = 2;
const int PlayerInventoryComponent::TOG_BUTTON_OPEN = 3;
const int PlayerInventoryComponent::TOG_BUTTON_OPEN_HOVER = 4;

PlayerInventoryComponent::PlayerInventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos) : InventoryComponent(p_iSize, p_iSlotsPerRow, p_v2DrawPos) {
    m_enType = PLAYER_INVENTORY;

    // If this is the first PlayerInventory
    if (!m_pToggleTexture) {
        // We need to initalize the shared toggle button texture
        wolf::Texture* pNewTexture = wolf::TextureManager::CreateTexture(m_strToggleTexturePath);
        if (pNewTexture) {
            if ((pNewTexture->GetWidth() * pNewTexture->GetHeight()) % (int)(m_v2TexFrameSize.x * m_v2TexFrameSize.y) != 0) {
                // If the new texture doesn't match the frame size then we delete it and leave the current texture unchanged
                wolf::TextureManager::DestroyTexture(pNewTexture);
                wolf::Error("Incorrectly sized texture file \"", m_strToggleTexturePath, "\" passed to InventoryComponent.");
            }

            // Then we need to know how many frames are in the texture
            int iNumFramesX = pNewTexture->GetWidth() / m_v2TexFrameSize.x;
            int iNumFramesY = pNewTexture->GetHeight() / m_v2TexFrameSize.y;

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
            m_vv2ToggleTextureCoords.push_back(pOffsetCoord);

            // Now that we have all our UV coordinates, we're going to assign them to frames
            for (int p = 0; p <= iNumFramesY - 1; p++) {
                for (int q = 0; q <= iNumFramesX - 1; q++) {
                    int iOriginPoint = (p * iWidth) + q;

                    // First we find the four UV coordinates that will be used to render this frame
                    // and store them in a struct that holds four glm::vec2s
                    ImGuiUVSet* pTexFrameCords = new ImGuiUVSet(av2WorkingUVCoords[iOriginPoint], av2WorkingUVCoords[iOriginPoint + iWidth + 1]);

                    // Then we store 'em
                    m_vv2ToggleTextureCoords.push_back(pTexFrameCords);
                }
            }

            pNewTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
            m_pToggleTexture = pNewTexture;
        }
    }

    // Display the closed button
    m_iToggleButtonIndex = TOG_BUTTON_CLOSED;

    // We need to fill the equipment array with nullptrs because we don't have anything equipped, yet
    for (int i = 0; i < END_OF_EQUIPMENT; i++) {
        m_pEquipment[i] = nullptr;
    }

    // We need to fill the schematics array with zeros because we don't have any schematics, yet
    for (int j = 0; j < END_OF_RARITIES; j++) {
        m_iSchematics[j] = 0;
    }

    // Start with exactly one common schematic
    m_iSchematics[0] = 1;

    // We also need to register for events related to the player's inventory
    wolf::EventManager::AddListener<OpenInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::AddListener<CloseInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleCloseInventoryEvent>(*this);
    wolf::EventManager::AddListener<SellItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleSellItemToPlayerEvent>(*this);
    wolf::EventManager::AddListener<PickupDroppedItemEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandlePickupDroppedItemEvent>(*this);
    wolf::EventManager::AddListener<EndPlacingPlaceableEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleEndPlacingPlaceableEvent>(*this);
    wolf::EventManager::AddListener<DispenseItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDispenseItemToPlayerEvent>(*this);
    wolf::EventManager::AddListener<SendItemToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
    wolf::EventManager::AddListener<RemoveFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerInventoryEvent>(*this);
    wolf::EventManager::AddListener<RemoveFromPlayerEquipmentEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerEquipmentEvent>(*this);
    
}

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
    wolf::EventManager::RemoveListener<EndPlacingPlaceableEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleEndPlacingPlaceableEvent>(*this);
    wolf::EventManager::RemoveListener<DispenseItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDispenseItemToPlayerEvent>(*this);
    wolf::EventManager::RemoveListener<SendItemToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<RemoveFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<RemoveFromPlayerEquipmentEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerEquipmentEvent>(*this);
}

void PlayerInventoryComponent::ShowToggleButtonGUI() {
    //  Prevents the player clicking on a tile position that overlaps the inventory button
    if(m_bIsPlacing) return;
    ImGuiStyle* pStyle = &ImGui::GetStyle();
    pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // You can't resize the inventory or move it
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;

    ImVec2 v2DisplaySize = ImGui::GetIO().DisplaySize;
    ImVec2 v2ButtonDrawPos = {10.0f, v2DisplaySize.y - 100.0f};

    // By default, the inventory appears close to the middle of the screen
    ImGui::SetNextWindowPos(v2ButtonDrawPos);
    ImGui::SetNextWindowSize({0,0});
    ImGui::Begin("InventoryToggleButton", nullptr, flags);

    // Get rid of ImGui's automatic button coloring so we can do it by swapping textures
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));

    // Scale the button
    ImVec2 v2ButtonSize = {m_v2TexFrameSize.x * 2.0f, m_v2TexFrameSize.y * 2.0f};

    // Create the button
    if (ImGui::ImageButton("InventoryToggleButton", (void*)(intptr_t)m_pToggleTexture->GetID(), v2ButtonSize, m_vv2ToggleTextureCoords[m_iToggleButtonIndex]->m_v2TopLeft, m_vv2ToggleTextureCoords[m_iToggleButtonIndex]->m_v2BotRight)) {
        this->ToggleOpen();
    }

    // Update whether the image button is hovered or not
    m_bToggleButtonHovered = ImGui::IsItemHovered();

    // If we are hovering over the button
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        if (m_bIsOpen) {
            m_iToggleButtonIndex = TOG_BUTTON_OPEN_HOVER;
        }
        else {
            m_iToggleButtonIndex = TOG_BUTTON_CLOSED_HOVER;
        }
    }
    else {
        if (m_bIsOpen) {
            m_iToggleButtonIndex = TOG_BUTTON_OPEN;
        }
        else {
            m_iToggleButtonIndex = TOG_BUTTON_CLOSED;
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
}

void PlayerInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

        // Position the inventory
        ImVec2 v2DisplaySize = ImGui::GetIO().DisplaySize;
        ImVec2 v2WindowDrawPos = {10.0f, 256};
        
        ImGui::SetNextWindowPos(v2WindowDrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin("~ Inventory ~", &m_bIsOpen, flags);

        // Get the size of the window
        ImVec2 v2WindowSize = ImGui::GetWindowSize();

        // So that we can calculate how big the background image needs to be
        ImVec2 v2BGMin = {v2WindowDrawPos.x, v2WindowDrawPos.y};
        ImVec2 v2BGMax = {v2WindowDrawPos.x + v2WindowSize.x, v2WindowDrawPos.y + v2WindowSize.y};

        // And then create the background image
        ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)m_pFrameTexture->GetID(), v2BGMin, v2BGMax, m_vv2FrameTextureCoords[1]->m_v2TopLeft, m_vv2FrameTextureCoords[1]->m_v2BotRight);

        float fWindowWidth = ImGui::GetWindowSize().x;
        float fWindowHeight = ImGui::GetWindowSize().y;

        // Push the colors for the X button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.f, 0.f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.f, 0.f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 0.f, 0.f, 0.75f));

        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
        if (ImGui::Button("X", ImVec2(fWindowWidth * 0.10f, fWindowHeight * 0.064f))) {
            this->Close();
        }

        ImGui::PopStyleColor(3);

        // Write the inventory title
        float fTextWidth = ImGui::CalcTextSize("~ Inventory ~").x;

        ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
        ImGui::Text("~ Inventory ~");

        // This counter lets us control how many items are drawn in a row
        int counter = 0;

        // We need to draw m_iSize number of slots
        for (int k = 0; k < m_iSize; k++) {
            // If this is the first slot in this row
            if (counter == 0) {
                // Draw a single character for padding
                ImGui::Text(" ");
                ImGui::SameLine();
            }

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

                // Push some style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

                // Draw the actual button
                if (ImGui::ImageButton("Filled Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2BotRight)) {
                }

                // And then pop the vars
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);
                
                // Push the tooltip style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

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

                // Pop the tooltip style vars and colors
                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(1);

                // When we click on an inventory slot
                if (ImGui::IsItemClicked()) {

                    // And then open a little pop-up menu
                    ImGui::OpenPopup(strIndex.c_str());
                }
                
                // Push the pop-up style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

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
                    else if (pItem->GetID() == PLACEABLE) {
                        if (ImGui::Button("Place")) {
                            m_iCurrentPlaceableIndex = k;
                            this->BeginPlacingPlaceable(pItem);
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

                // Pop the pop-up style vars and colors
                ImGui::PopStyleVar(1);
                ImGui::PopStyleColor(5);

            }
            else { // Otherwise, this is an empty inventory slot
                // Push some style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

                // Draw the button
                if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {

                }

                // And pop the style vars and colors
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);
            }

            // If we've drawn the maximum number of slots per row
            if (counter == m_iMaxPerRow - 1) {
                // Reset the counter
                counter = 0;

                // And draw a single character for padding
                ImGui::SameLine();
                ImGui::Text(" ");
            }
            else {
                // Otherwise, this slot needs to be drawn on the same line as the last one
                ImGui::SameLine();
                counter++;
            }
        }

        // Reset the counter because we're moving on to a new section of the inventory
        counter = 0;

        // Draw the equipped items
        fTextWidth = ImGui::CalcTextSize("Equipment").x;
        ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
        ImGui::Text("Equipment");

        for (int t = 0; t < END_OF_EQUIPMENT; t++) {
            // If this is the first slot in this row
            if (counter == 0) {
                // Draw a single character for padding
                ImGui::Text(" ");
                ImGui::SameLine();
            }

            // Get the item equipped in this slot
            EquipmentItem* pEquipItem = m_pEquipment[t];

            // If there is an item equipped in the slot
            if (pEquipItem) {
                // Get the name and item details
                std::string strEquipName = pEquipItem->GetName();
                std::string strEquipTooltip = pEquipItem->GetToolTipText();

                // Push some style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

                // Draw the inventory slot
                if (ImGui::ImageButton("Equipment Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[pEquipItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2ItemTextureCoords[pEquipItem->GetTextureFrameIndex()]->m_v2BotRight)) {
                }

                // Pop the style vars and colors
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);

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
            else { // Otherwise, this slot is empty
                // Push some style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

                // Draw the button
                if (ImGui::ImageButton("Empty Equipment Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {
                }

                // Pop the style vars and colors
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);
            }

            // If we've drawn the maximum number of slots per row
            if (counter == m_iMaxPerRow - 1) {
                // Reset the counter
                counter = 0;

                // Draw a single character for padding
                ImGui::SameLine();
                ImGui::Text(" ");
            }
            else {
                // Otherwise, this slot needs to be drawn on the same line as the last one
                ImGui::SameLine();
                counter++;
            }
        }

        // Iterate through the schematic counters
        ImGui::Text("  Schematics:");
        for (int p = 0; p < END_OF_RARITIES; p++) {
            // Retrieve the color associated with this rarity
            RGBIntColor color = RarityColors[p];

            // And draw the counter's value in that color
            ImGui::SameLine();
            ImGui::TextColored(ImColor(color.r, color.g, color.b), "%d", m_iSchematics[p]);
        }

        // Then draw a single character for padding
        ImGui::SameLine();
        ImGui::Text(" ");

        // Draw the player's gold value
        ImGui::Text("  Gold:");
        ImGui::SameLine();
        ImGui::TextColored(ImColor(255, 215, 0), "%d ", m_iGold); // in gold (ha)

        ImGui::NewLine();

        // End of window
        ImGui::End();

        // If our pockets are full and we tried to add an item to 'em
        if (m_bShowFullInventoryPrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // Push the message style vars and colors
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

            float fMessageWidth = ImGui::CalcTextSize("Your inventory is full.").x;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({v2DisplaySize.x * 0.5f - (fMessageWidth / 2.0f), v2DisplaySize.y * 0.45f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Inventory Is Full Prompt", nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

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

            // Pop the style vars and colors
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(4);

            ImGui::End();
        }

        // If we tried to sell something to a merchant but they can't afford it
        if (m_bShowTooExpensivePrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // Push the message style vars and colors
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

            float fTooExpensiveWidth = ImGui::CalcTextSize("You don't have enough gold to buy that.").x;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({v2DisplaySize.x * 0.5f - (fTooExpensiveWidth / 2.0f), v2DisplaySize.y * 0.45f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Too Expensive Prompt", nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

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

            // Pop the message style vars and colors
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(4);

            ImGui::End();
        }

        // If we tried to get an item that we don't have a schematic for from a dispensary
        if (m_bShowMissingSchematicPrompt) {
            // You can't resize the window or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // Push the message style vars and colors
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

            float fMissingSchematicWidth = ImGui::CalcTextSize("You don't have a schematic to trade for that.").x;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({v2DisplaySize.x * 0.5f - (fMissingSchematicWidth / 2.0f), v2DisplaySize.y * 0.45f});
            ImGui::SetNextWindowSize({0, 0});
            ImGui::Begin("Missing Schematic Prompt", nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

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

            // Pop style vars and colors
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(4);

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

void PlayerInventoryComponent::BeginPlacingPlaceable(ItemBase* p_pItem)
{
    PlaceableItem* pPlaceable = dynamic_cast<PlaceableItem*>(p_pItem);
    if (pPlaceable){
        m_bIsPlacing = true;
        wolf::EventManager::TriggerEvent(BeginPlacingPlaceableEvent(pPlaceable));
        Close();
    }
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
        m_bIsOpen = false;
    }
    else if (p_event.enType == MERCHANT_INVENTORY && p_event.iIdNum == m_iOpenMerchantIdNum) {
        // We do the same with merchant inventories
        m_iOpenMerchantIdNum = -1;
        m_bIsOpen = false;
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

void PlayerInventoryComponent::HandleEndPlacingPlaceableEvent(const EndPlacingPlaceableEvent& p_event)
{
    RemoveItem(m_iCurrentPlaceableIndex);
    m_iCurrentPlaceableIndex = -1;
    m_bIsPlacing = false;
    Open();
}