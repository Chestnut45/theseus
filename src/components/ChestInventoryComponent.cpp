#include "ChestInventoryComponent.h"

//-----------------------------------------------------------------------------
// File:            ChestInventoryComponent.cpp
// Original Author: Aurora Ryder
//
// A class representing a given chest's inventory
//-----------------------------------------------------------------------------

#include <yaml-cpp/yaml.h>
#include "../inventory/ItemCreator.h"

#include <AnimatedSprite2D.h>
#include <LightEvents.h>


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

// Fills the chest using the given rng to choose items from a .yaml loot table
// > filepath: path to the .yaml loot table
// > rng: random number generator that will be used to randomly select items from the loot table
bool ChestInventoryComponent::FillFromLootTable(const std::string& filepath, wolf::RNG& rng)
{
    try
    {
        // Load the file
        YAML::Node node = YAML::LoadFile(filepath);

        // Get the number of items if it exists
        int numItems = node["items"] ? node["items"].as<int>() : 0;

        // If it doesn't exist, calculate it from min and max
        if (numItems == 0)
        {
            numItems = rng.NextInt(node["min_items"].as<int>(), node["max_items"].as<int>());
        }

        // Grab the loot table node
        YAML::Node lootTable = node["loot_table"];
        int numEntries = lootTable.size();

        // Initialize an array of entries
        YAML::Node entries[numEntries];

        // Count the number of items in the loot table
        float sumWeights = 0;
        for (int i = 0; i < numEntries; ++i)
        {
            entries[i] = lootTable[i];
            const YAML::Node& entry = entries[i];
            sumWeights += entry["probability"].as<float>();
        }

        for (int i = 0; i < numItems; ++i)
        {
            // Generate a random number
            float value = rng.NextFloat(0.0f, sumWeights);
            std::string chosenItem;
            for (int j = 0; j < numEntries; ++j)
            {
                float probability = entries[j]["probability"].as<float>();
                
                if (value < probability)
                {
                    chosenItem = entries[j]["name"].as<std::string>();
                    break;
                }
                value -= probability;
            }

            // Create the item
            ItemBase* pItem = ItemCreator::CreateItem(chosenItem);
            if (!pItem)
            {
                wolf::Error("Item name invalid: ", chosenItem);
                continue;
            }

            // Add the item
            AddItemOrDelete(pItem);
        }
    }
    catch (YAML::Exception& e)
    {
        // If we run into an error, then we should print it and return false
        wolf::Error("YAML: Issue with ", filepath.c_str(), ": ", e.what());
        return false;
    }

    // If we didn't encounter any issues, we return true
    return true;
}

// Displays the chest's GUI
void ChestInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

        // Position the inventory
        ImVec2 v2DisplaySize = ImGui::GetIO().DisplaySize;
        ImVec2 v2WindowDrawPos = {v2DisplaySize.x - (7 * m_v2TexFrameSize.x), 256};

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(v2WindowDrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin("~ Chest ~", &m_bIsOpen, flags);

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
        float fTextWidth = ImGui::CalcTextSize("~ Chest ~").x;

        ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
        ImGui::Text("~ Chest ~");

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

                    // Then construct the string that will be used to display the item's name
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

                    // If this item is equipped then we want to show that in the details string
                    if (pEquipment->IsEquipped()) {
                        strTooltipName += " (E)";
                        bIsEquipped = true; // (And we'll need to remember that it's equipped later on)
                    }
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

                // Now we can start making the actual buttons
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
                    // We open a little pop-up menu
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

        // Newline for padding
        ImGui::NewLine();

        // End of window
        ImGui::End();
    }
}

// Sends the item stored at p_iItemIndex to the player by triggering a SendItemToPlayerInventoryEvent
void ChestInventoryComponent::SendItemToPlayer(int p_iItemIndex) {
    // Retrieve the item from the inventory and send it to the player via an event.
    wolf::EventManager::TriggerEvent(SendItemToPlayerInventoryEvent(m_enType, m_iIdNum, this->GetItem(p_iItemIndex), p_iItemIndex));
    
    // !-- Note that we also send along the index that this specific chest was storing the item at to make sure
    // that when we the player tells us to remove the item from the chest later we can remove the exact item that
    // we sent rather than the first instance of it (in case we have multiple items with the same name) --!
}

// Handler for OpenInventoryEvents that will close this chest if another inventory opens
// p_event: the OpenInventoryEvent object
void ChestInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this chest is open
    if (m_bIsOpen) {
        // And a different inventory that ISN'T the player's is opening
        if (p_event.enType != PLAYER_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;

            // Adjust sprite
            auto* pAnim = GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim)
            {
                std::string name = pAnim->GetCurrentAnimation()->m_strName;
                size_t pos = name.find("Open");
                if (pos != std::string::npos) pAnim->SetAnimation(name.replace(pos, 4, "Closed"));

                if (this->IsEmpty()) {
                    wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), false));
                }
                else {
                    wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), true));
                }
            }
        }
    }
}

// Handler for CloseInventoryEvents that will close this chest if it was open and the player closed their inventory
// > p_event: the CloseInventoryEvent object
void ChestInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If the player just closed their inventory
    if (p_event.enType == PLAYER_INVENTORY || p_event.enType == CHEST_INVENTORY) {
        // And this chest is open
        if (m_bIsOpen) {
            // Close it
            m_bIsOpen = false;

            // Adjust sprite
            auto* pAnim = GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim)
            {
                std::string name = pAnim->GetCurrentAnimation()->m_strName;
                size_t pos = name.find("Open");
                if (pos != std::string::npos) pAnim->SetAnimation(name.replace(pos, 4, "Closed"));

                if (this->IsEmpty()) {
                    wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), false));
                }
                else {
                    wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), true));
                }
            }
        }
    }
}

// Handler for AddToChestEvents that will add an item to the chest as long is there is space to hold it.
// Otherwise, the item will be returned to the player
// > p_event: the AddToChestEvent object
void ChestInventoryComponent::HandleAddToChestEvent(const SendItemToChestEvent& p_event) {
    // If the player is trying to add an item to this specific chest
    if (p_event.iChestIdNum == m_iIdNum) {
        // And we have the space to hold it
        if (this->AddItem(p_event.pItem)) {
            // Then we need to check if the player just stored an item that they had equipped
            if (p_event.bWasEquipped) {
                // If it was equipped, we need to trigger a RemoveFromPlayerEquipmentEvent
                wolf::EventManager::TriggerEvent(RemoveFromPlayerEquipmentEvent(p_event.iPlayerInventoryIndex));
            }
            else {
                // Otherwise, we need to trigger a RemoveFromPlayerInventoryEvent
                wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(p_event.pItem->GetName(), p_event.iPlayerInventoryIndex));
            }
        }
    }
}

// Handler for RemoveFromChestEvents that will remove an item from this chest
// > p_event: the RemoveFromChestEvent object
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