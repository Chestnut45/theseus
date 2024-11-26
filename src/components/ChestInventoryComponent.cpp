#include "ChestInventoryComponent.h"

#include <yaml-cpp/yaml.h>
#include "../inventory/ItemCreator.h"

#include <AnimatedSprite2D.h>

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

void ChestInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(m_v2DrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin("~ Chest ~", &m_bIsOpen, flags);

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

                    // Then construct the string that will be used to display all of the item's details
                    strTooltipName = pConsumable->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")";
                    strTooltipText = pConsumable->GetDescription() + "\n\nValue: " + std::to_string(pConsumable->GetValue())
                        + "\nUses: " + std::to_string(pConsumable->GetNumUses());

                }
                else if (pItem->GetID() == EQUIPMENT) { // If this is an equipment item
                    // Try to cast it
                    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
                    if (!pEquipment) {
                        // And throw an error if we couldn't
                        wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                    }
                    
                    // Then start constructing the string that will be used to display all of the item's details
                    strTooltipName = pEquipment->GetName();

                    // If this item is equipped then we want to show that in the details string
                    if (pEquipment->IsEquipped()) {
                        strTooltipName += " (E)";
                        bIsEquipped = true; // (And we'll need to remember that it's equipped later on)
                    }

                    // Add the rest of the item's details to the string
                    strTooltipText = pEquipment->GetDescription() + "\n\nValue: " + std::to_string(pEquipment->GetValue())
                        + "\nSlot: " + pEquipment->GetEquipmentSlotString();
                }
                else { // If for some reason this item isn't Consumable OR Equipment
                    strTooltipName = pItem->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")";
                    strTooltipText = pItem->GetDescription();
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

        // End of window
        ImGui::End();
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

            // Adjust sprite
            auto* pAnim = GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim)
            {
                std::string name = pAnim->GetCurrentAnimation()->m_strName;
                size_t pos = name.find("Open");
                if (pos != std::string::npos) pAnim->SetAnimation(name.replace(pos, 4, "Closed"));
            }
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

            // Adjust sprite
            auto* pAnim = GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim)
            {
                std::string name = pAnim->GetCurrentAnimation()->m_strName;
                size_t pos = name.find("Open");
                if (pos != std::string::npos) pAnim->SetAnimation(name.replace(pos, 4, "Closed"));
            }
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