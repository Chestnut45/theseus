#include "DispensaryInventoryComponent.h"

//-----------------------------------------------------------------------------
// File:            DispensaryInventoryComponent.cpp
// Original Author: Aurora Ryder
//
// A class representing a armament dispensary inventory
//-----------------------------------------------------------------------------

#include <ItemCreator.h>
#include <AnimatedSprite2D.h>

#include <W_Audio.h>
#include <LightEvents.h>

DispensaryInventoryComponent::~DispensaryInventoryComponent() {
    // Deregister for events
    wolf::EventManager::RemoveListener<OpenInventoryEvent, DispensaryInventoryComponent, &DispensaryInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::RemoveListener<CloseInventoryEvent, DispensaryInventoryComponent, &DispensaryInventoryComponent::HandleCloseInventoryEvent>(*this);

    // Dispensary inventories need to be emptied a bit differently than other inventories
    // so we use an overloaded version of the EmptyInventory method
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();
}

/* AVOID USING THIS METHOD WHEN POSSIBLE. Dispensary contents aren't really
// meant to change once they've been initialized, and FillInventoryFromFile
// calls this method internally, so there shouldn't really be a reason to
// manually empty a dispensary inventory. */
void DispensaryInventoryComponent::EmptyInventory() {
    // Empty the inventory as usual
    InventoryComponent::EmptyInventory();

    // Then empty all of the rarity index vectors so that we
    // aren't holding onto indices that don't have anything in 'em
    for (int k = 0; k < END_OF_RARITIES; k++) {
        m_arContentsByRarity[k].clear();
    }
}

// AVOID USING THIS METHOD WHEN POSSIBLE. If you add an item to a dispensary inventory it
// needs to be resorted which can take a bit of time. This method should only be used by
// the FillInventoryFromFile method.
bool DispensaryInventoryComponent::AddItem(ItemBase* p_pItem) {
    bool result = InventoryComponent::AddItem(p_pItem);
    m_bUnsorted = result;
    return result;
}

// AVOID USING THIS METHOD WHEN POSSIBLE. If you remove an item to a dispensary inventory it
// needs to be resorted which can take a bit of time. Dispensary contents should NOT change
// after intialization via the FillInventoryFromFile method.
bool DispensaryInventoryComponent::RemoveItem(const std::string& p_strItemName) {
    bool result = InventoryComponent::RemoveItem(p_strItemName);
    m_bUnsorted = result;
    return result;
}

// AVOID USING THIS METHOD WHEN POSSIBLE. If you remove an item to a dispensary inventory it
// needs to be resorted which can take a bit of time. Dispensary contents should NOT change
// after intialization via the FillInventoryFromFile method.
bool DispensaryInventoryComponent::RemoveItem(ItemID p_enItemID) {
    bool result = InventoryComponent::RemoveItem(p_enItemID);
    m_bUnsorted = result;
    return result;
}

// AVOID USING THIS METHOD WHEN POSSIBLE. If you remove an item to a dispensary inventory it
// needs to be resorted which can take a bit of time. Dispensary contents should NOT change
// after intialization via the FillInventoryFromFile method.
bool DispensaryInventoryComponent::RemoveItem(int p_iItemIndex) {
    bool result = InventoryComponent::RemoveItem(p_iItemIndex);
    m_bUnsorted = result;
    return result;
}

// Fills the dispensary with using a .yaml loot table
// > p_strFilePath: path to the .yaml loot table
bool DispensaryInventoryComponent::FillInventoryFromFile(const std::string& p_strFilePath) {
    // If the inventory has things in it already
    if (!this->IsEmpty()) {
        // Empty it out
        this->EmptyInventory();
    }

    // Then attempt to fill the inventory and sort its contents
    bool result = InventoryComponent::FillInventoryFromFile(p_strFilePath);
    this->SortByRarity();

    // Return the result
    return result;
}

// Sorts the contents of this DispensaryInventory by their rarity level
void DispensaryInventoryComponent::SortByRarity() {
    // Clear out the rarity vectors
    for (int i = 0; i < END_OF_RARITIES; i++) {
        m_arContentsByRarity[i].clear();
    }

    // Then go through the contents of the inventory
    for (int j = 0; j < m_vvpContents.size(); j++) {
        // Get the item at each index
        if (!m_vvpContents[j].empty()) {
            ItemBase* pItem = m_vvpContents[j].top();
            if (pItem) {
                // And save its index to the corresponding rarity vectory
                m_arContentsByRarity[pItem->GetRarity()].push_back(j);
            }
        }
    }

    // And set unsorted to false
    m_bUnsorted = false;
}

// Shows the dispensary's GUI
void DispensaryInventoryComponent::ShowInventoryGUI() {
    // If the dispensary is open
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);
        
        // If the inventory contents need to be sorted again
        if (m_bUnsorted) {
            // Do that
            this->SortByRarity();
        }

        // You can't resize the inventory but you can move it around!
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

        // Position the inventory
        ImVec2 v2DisplaySize = ImGui::GetIO().DisplaySize;
        ImVec2 v2WindowDrawPos = {v2DisplaySize.x - (7 * m_v2TexFrameSize.x), 256};

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(v2WindowDrawPos);
        ImGui::SetNextWindowSize({0, 0});
        ImGui::Begin("~ Dispensary ~", nullptr, flags);

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
        float fTextWidth = ImGui::CalcTextSize("~ Daedalus Dispensary ~").x;

        ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
        ImGui::Text("~ Daedalus Dispensary ~");

        RGBIntColor color = RarityColors[COMMON];

        // Go through the list of rarities
        for (int i = 0; i < END_OF_RARITIES; i++) {
            // Each time we start a new rarity, create a new UI section beneath the rarity
            switch(i) {
                case COMMON:;
                    // Retrieve the color associated with this rarity
                    color = RarityColors[COMMON];

                    // And draw the counter's value in that color
                    ImGui::TextColored(ImColor(color.r, color.g, color.b), "  COMMON");

                    ImGui::Text(" ");
                    ImGui::SameLine();
                break;
                case UNCOMMON:
                    color = RarityColors[UNCOMMON];
                    ImGui::TextColored(ImColor(color.r, color.g, color.b), "  UNCOMMON");

                    ImGui::Text(" ");
                    ImGui::SameLine();
                break;
                case RARE:
                    color = RarityColors[RARE];
                    ImGui::TextColored(ImColor(color.r, color.g, color.b), "  RARE");

                    ImGui::Text(" ");
                    ImGui::SameLine();
                break;
                case EPIC:
                    color = RarityColors[EPIC];
                    ImGui::TextColored(ImColor(color.r, color.g, color.b), "  EPIC");

                    ImGui::Text(" ");
                    ImGui::SameLine();
                break;
                case LEGENDARY:
                    color = RarityColors[LEGENDARY];
                    ImGui::TextColored(ImColor(color.r, color.g, color.b), "  LEGENDARY");

                    ImGui::Text(" ");
                    ImGui::SameLine();
                break;
            }

            // This counter makes sure that we do not have rows of items that are larger than the maximum
            int counter = 0;

            // For each of the rarity item index lists
            std::vector<int> viRarityIndices = m_arContentsByRarity[i];
            for (auto index : viRarityIndices) {
                // Get the item at each index
                ItemBase* pItem = this->GetItem(index);

                // And create a tooltip out of the item's information
                std::string strTooltipName;
                std::string strTooltipText = pItem->GetToolTipText();

                // Every item in this inventory SHOULD be an equipment item, so we try to cast it
                EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
                if (!pEquipment) {
                    // And throw an error if we couldn't
                    wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                }
                    
                // Then start constructing the string that will be used to the item's name
                strTooltipName = pEquipment->GetName();
                std::string strIndex = std::to_string(index);

                // Push some style vars and colors
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

                // Then we make a button (UI inventory slot) for the item
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

                    // Set the child's animation to be the item's icon
                    for (auto& child : this->GetGameObject()->GetChildren()) {
                        AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                        if (anim) {
                            anim->SetAnimation(std::to_string(pItem->GetTextureFrameIndex()).c_str());
                        }
                    }
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
                if (ImGui::BeginPopup(strIndex.c_str(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
                    if (ImGui::Button("Trade")) {
                        DispenseItem(index);
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

            // Push some style vars and colors for the empty buttons
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 50.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

            // We want to make sure that the rows of items in the dispensary are uniform even if we have different numbers of items
            // at each rarity level, so if a rarity level doesn't use an entire row we fill the remaining space with empty slots
            for (int p = counter; p != m_iMaxPerRow; p++) {
                if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {}
                ImGui::SameLine();
            }

            // Draw a newline character for padding
            ImGui::Text(" ");

            // And pop the style vars and colors
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
        }

        // Draw a newline for padding
        ImGui::Text(" ");
        ImGui::NewLine();

        ImGui::End();
    }
}

// Removes the item stored at p_iItemIndex from the dispensary's inventory
// and sends it to the player using a DispenseItemToPlayerEvent
// > p_iItemIndex: the index of the item that will be dispensed
void DispensaryInventoryComponent::DispenseItem(int p_iItemIndex) {
    // When we dispense an item, we need to make a new instance of an item that is in the dispensary
    ItemBase* pItemToDispense = this->GetItem(p_iItemIndex);

    // So we try to get the requested item
    if (pItemToDispense) {
        // Then we make a "copy" (new instance) of the item using the item creator
        ItemBase* pCopyOfItemToDispense = ItemCreator::CreateItem(pItemToDispense->GetName());

        // And we send the "copy" to the player
        wolf::EventManager::TriggerEvent(DispenseItemToPlayerEvent(m_iIdNum, pCopyOfItemToDispense));
    }
}

// Handler for OpenInventoryEvents that plays the dispensary startup animation and sound
// > p_event: the OpenInventoryEvent object
void DispensaryInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {

    if (p_event.enType == InventoryType::DISPENSARY_INVENTORY && p_event.iIdNum == m_iIdNum)
    {
        wolf::Audio::Play("data/sounds/sfx_dispensary_open.wav", 1.0f);
        wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), true));
    }

    // If this dispensary is open
    if (m_bIsOpen) {
        // And a different inventory that ISN'T the player's was just opened
        if (p_event.enType != PLAYER_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
            AnimatedSprite2D* pAnim = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim) {
                pAnim->SetAnimation("Deactivate");
            }
            wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), false));
        }
    }
}

// Handler for CloseInventoryEvents that plays the dispensary shutdown sound and animation
// > p_event: the CloseInventoryEvent object
void DispensaryInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {

    bool sfxPlayed = false;
    if (p_event.enType == InventoryType::DISPENSARY_INVENTORY && p_event.iIdNum == m_iIdNum)
    {
        wolf::Audio::Play("data/sounds/sfx_dispensary_close.wav", 1.0f);
        sfxPlayed = true;
        wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), false));
    }

    // If this dispensary is open
    if (m_bIsOpen) {
        // And the player just closed their inventory
        if (p_event.enType == PLAYER_INVENTORY || p_event.enType == DISPENSARY_INVENTORY) {
            // Close the dispensary as well
            m_bIsOpen = false;
            AnimatedSprite2D* pAnim = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim) {
                pAnim->SetAnimation("Deactivate");
            }

            // Play SFX in edge case where dispensary inventory is closed by walking away
            if (!sfxPlayed) wolf::Audio::Play("data/sounds/sfx_dispensary_close.wav", 1.0f);

            // Hide the child icon
            for (auto& child : this->GetGameObject()->GetChildren()) {
                AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                if (anim) {
                    anim->SetAnimation("Transparent");
                }
            }

            wolf::EventManager::TriggerEvent(LightToggleEvent(this->GetGameObject()->GetID(), false));
        }
    }
}