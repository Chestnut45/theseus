#include "DispensaryInventoryComponent.h"
#include "inventory/ItemCreator.h"
#include "AnimatedSprite2D.h"

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

// AVOID USING THIS METHOD WHEN POSSIBLE. Dispensary contents aren't really
// meant to change once they've been initialized and FillInventoryFromFile
// calls this method internally, so there shouldn't really be a reason to
// manually empty a dispensary inventory.
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
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(m_v2DrawPos);
        ImGui::SetNextWindowSize({0, 0});
        ImGui::Begin("~ Dispensary ~", nullptr, flags);

        // Go through the list of rarities
        for (int i = 0; i < END_OF_RARITIES; i++) {
            // Each time we start a new rarity, create a new UI section beneath the rarity
            switch(i) {
                case COMMON:
                    ImGui::SeparatorText("COMMON");
                break;
                case UNCOMMON:
                    ImGui::SeparatorText("UNCOMMON");
                break;
                case RARE:
                    ImGui::SeparatorText("RARE");
                break;
                case EPIC:
                    ImGui::SeparatorText("EPIC");
                break;
                case LEGENDARY:
                    ImGui::SeparatorText("LEGENDARY");
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

                // Then we make a button (UI inventory slot) for the item
                if (ImGui::ImageButton("Filled Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2BotRight)) {
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

                    // Set the child's animation to be the item's icon
                    for (auto& child : this->GetGameObject()->GetChildren()) {
                        AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                        if (anim) {
                            anim->SetAnimation(std::to_string(pItem->GetTextureFrameIndex()).c_str());
                        }
                    }
                }

                // When we click on an inventory slot
                if (ImGui::IsItemClicked()) {
                    // We open a little pop-up menu
                    ImGui::OpenPopup(strIndex.c_str());
                }
                        
                // The pop-up menu has different buttons based on what the item is and what "state" it's in
                if (ImGui::BeginPopup(strIndex.c_str())) {
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

            // We want to make sure that the rows of items in the dispensary are uniform even if we have different numbers of items
            // at each rarity level, so if a rarity level doesn't use an entire row we fill the remaining space with empty slots
            for (int p = counter; p != m_iMaxPerRow; p++) {
                if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2ItemTextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {}
                ImGui::SameLine();
            }

            // Then we start a new line for the next rarity level
            ImGui::NewLine();
        }

        ImGui::End();
    }
}

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

void DispensaryInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this dispensary is open
    if (m_bIsOpen) {
        // And a different dispensary was just opened
        if (p_event.enType == DISPENSARY_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
            AnimatedSprite2D* pAnim = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim) {
                pAnim->SetAnimation("Deactivate");
            }
        }
    }
}

void DispensaryInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If this dispensary is open
    if (m_bIsOpen) {
        // And the player just closed their inventory
        if (p_event.enType == PLAYER_INVENTORY) {
            // Close the dispensary as well
            m_bIsOpen = false;
            AnimatedSprite2D* pAnim = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
            if (pAnim) {
                pAnim->SetAnimation("Deactivate");
            }

            // Hide the child icon
            for (auto& child : this->GetGameObject()->GetChildren()) {
                AnimatedSprite2D* anim = child->GetComponent<AnimatedSprite2D>();
                if (anim) {
                    anim->SetAnimation("Transparent");
                }
            }
        }
    }
}