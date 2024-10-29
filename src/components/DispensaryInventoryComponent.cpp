#include "DispensaryInventoryComponent.h"

DispensaryInventoryComponent::~DispensaryInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();
}

bool DispensaryInventoryComponent::AddItem(ItemBase* p_pItem) {
    bool result = InventoryComponent::AddItem(p_pItem);
    m_bUnsorted = result;
    return result;
}

bool DispensaryInventoryComponent::RemoveItem(const std::string& p_strItemName) {
    bool result = InventoryComponent::RemoveItem(p_strItemName);
    m_bUnsorted = result;
    return result;
}

bool DispensaryInventoryComponent::RemoveItem(ItemID p_enItemID) {
    bool result = InventoryComponent::RemoveItem(p_enItemID);
    m_bUnsorted = result;
    return result;
}

bool DispensaryInventoryComponent::RemoveItem(int p_iItemIndex) {
    bool result = InventoryComponent::RemoveItem(p_iItemIndex);
    m_bUnsorted = result;
    return result;
}

bool DispensaryInventoryComponent::FillInventoryFromFile(const std::string& p_strFilePath) {
    bool result = InventoryComponent::FillInventoryFromFile(p_strFilePath);
    this->SortByRarity();

    return result;
}

void DispensaryInventoryComponent::SortByRarity() {
    // Clear out the rarity vectors
    for (int i = 0; i < m_arContentsByRarity->size(); i++) {
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
        // If the inventory contents need to be sorted again
        if (m_bUnsorted) {
            // Do that
            this->SortByRarity();
        }

        // You can't resize the inventory but you can move it around!
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos({300, 200});
        ImGui::SetNextWindowSize({0, 0});
        ImGui::Begin("~ Dispensary ~", nullptr, flags);

        for (int i = 0; i < END_OF_RARITIES; i++) {
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

            int counter = 0;

            std::vector<int> viRarityIndices = m_arContentsByRarity[i];
            for (auto index : viRarityIndices) {
                ItemBase* pItem = this->GetItem(index);

                std::string strTooltipText;

                if (pItem->GetID() == EQUIPMENT) { // If this is an equipment item
                    // Try to cast it
                    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
                    if (!pEquipment) {
                        // And throw an error if we couldn't
                        wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                    }
                    
                    // Then start constructing the string that will be used to display all of the item's details
                    strTooltipText = pEquipment->GetName() + "\n\n" + pEquipment->GetDescription() + "\n\nValue: " 
                        + std::to_string(pEquipment->GetValue()) + "\nSlot: " + pEquipment->GetEquipmentSlotString();

                    std::string strIndex = std::to_string(index);

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

            ImGui::NewLine();
        }

        ImGui::End();
    }
}

void DispensaryInventoryComponent::DispenseItem(int p_iItemIndex) {

}