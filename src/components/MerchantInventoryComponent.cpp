#include "MerchantInventoryComponent.h"

#include <yaml-cpp/yaml.h>
#include "inventory/ItemCreator.h"
#include "NPCComponent.h"

MerchantInventoryComponent::~MerchantInventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();

    // And deregister this event's listeners
    wolf::EventManager::AddListener<OpenInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleOpenInventoryEvent>(*this);
    wolf::EventManager::AddListener<CloseInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleCloseInventoryEvent>(*this);
    wolf::EventManager::AddListener<SellItemToMerchantEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleSellItemToMerchantEvent>(*this);
    wolf::EventManager::AddListener<BoughtItemFromMerchantEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleBoughtItemFromMerchantEvent>(*this);
}

// This method uses the same exact structure as the base class AddItem(ItemBase* p_pItem) method, but
// it does not add the item to the inventory, it only checks if the item *can* be added
bool MerchantInventoryComponent::CanAddItem(ItemBase* p_pItem) {
    // If we're not using any of the slots then we can just insert the item so we return true
    if (m_iSlotsInUse == 0) {
        return true;
    }

    // Otherwise we need to look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same ID and it is stackable
        if (pItem->GetID() == p_pItem->GetID() && p_pItem->IsStackable() && pItem->IsStackable()) {
            // Then we've got room for the item we want to add so we return true
            return true;
        }
    }

    // Otherwise, we check if there is an empty inventory slot to start a new item stack in
    if (m_iSlotsInUse < m_iSize) {
        // And return true if there is one
        return true;
    }

    // And if all that fails, then we can't fit the item so we return false
    return false;
}

void MerchantInventoryComponent::HandleOpenInventoryEvent(const OpenInventoryEvent& p_event) {
    // If this merchant is open
    if (m_bIsOpen) {
        // And another inventory that ISN'T the player's or this one is opening
        if (p_event.enType != PLAYER_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
        }
    }
}

void MerchantInventoryComponent::Close() {
    // Close the inventory and let anyone interested know it happened
    m_bIsOpen = false;
    wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));

    // Then check if there is an npc attached to our GameObject
    NPCComponent* pNPC = this->GetGameObject()->GetComponent<NPCComponent>();
    if (pNPC) {
        // If there is, tell them to say goodbye
        pNPC->SayGoodbye();
    }
}

void MerchantInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If the player just closed their inventory
    if (p_event.enType == PLAYER_INVENTORY) {
        // And this merchant is open
        if (m_bIsOpen) {
            // Close it
            m_bIsOpen = false;

            // Then check if there is an npc attached to our GameObject
            NPCComponent* pNPC = this->GetGameObject()->GetComponent<NPCComponent>();
            if (pNPC) {
                // If there is, tell them to say goodbye
                pNPC->SayGoodbye();
            }

            // Note that we do not use the Close() method to do this as we do not
            // want to send off another inventory closed event
        }
    }
}

void MerchantInventoryComponent::HandleSellItemToMerchantEvent(const SellItemToMerchantEvent& p_event) {
    // If the player is selling an item to this specific merchant
    if (p_event.iMerchantIdNum == m_iIdNum) {
        // If there is enough room in the merchant's inventory for the item
        if (CanAddItem(p_event.pItem)) {
            // If the merchant has enough gold to buy the item
            if (p_event.pItem->GetValue() <= m_iGold) {
                m_iGold -= p_event.pItem->GetValue(); // Take the gold from the merchant
                this->AddItem(p_event.pItem); // And add the item to their inventory

                // We then need to check if the item was equipped or not
                if (p_event.bWasEquipped) {
                    // If it was, we need to trigger a RemoveFromPlayerEquipmentEvent
                    wolf::EventManager::TriggerEvent(RemoveFromPlayerEquipmentEvent(p_event.iPlayerInventoryIndex, p_event.pItem->GetValue()));
                }
                else {
                    // Otherwise, we trigger a RemoveFromPlayerInventoryEvent
                    wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(p_event.pItem->GetName(), p_event.iPlayerInventoryIndex, p_event.pItem->GetValue()));
                }
            }
            else { // If the merchant does not have enough gold
                m_bShowSellForLessPrompt = true; // We need to ask the player if they are willing to sell the item, anyway

                // So we put the item that we're asking about in "stasis" so we can keep track of it outside of this method
                m_ItemInStasis.pItem = p_event.pItem;
                m_ItemInStasis.iPlayerInventoryIndex = p_event.iPlayerInventoryIndex;
                m_ItemInStasis.bWasEquipped = p_event.bWasEquipped;
            }
        }
    }
}

void MerchantInventoryComponent::HandleBoughtItemFromMerchantEvent(const BoughtItemFromMerchantEvent& p_event) {
    // If this is the merchant instance that the item was purchased from
    if (p_event.iMerchantIdNum == m_iIdNum) {
        // Check if we can remove the item using its index in this instance's inventory
        if (p_event.iMerchantInventoryIndex != -1) {
            // If we can, do so
            this->RemoveItem(p_event.iMerchantInventoryIndex);
        }
        else {
            // If we can't, remove the first item we find with a matching name
            this->RemoveItem(p_event.strItemName);
        }

        // Then add the gold we made from the sale to our wallet
        this->AddGold(p_event.iBoughtFor);
    }
}

void MerchantInventoryComponent::TakeItemOutOfStasis(bool p_bSold) {
    // If the item is being taken out of stasis because the player sold it,
    if (p_bSold) {
        // We need to add the item to our inventory
        this->AddItem(m_ItemInStasis.pItem);

        // Figure out how much gold we had at the time of sale
        int iSoldFor = this->GetGold();

        // Take away the gold we spent
        this->TakeGold(m_ItemInStasis.pItem->GetValue());

        // And let the player know the sale has gone through by triggering an event
        // based on whether or not the item was equipped when they sold it to us
        if (m_ItemInStasis.bWasEquipped) {
            // If it was equipped, trigger a RemoveFromPlayerEquipmentEvent
            wolf::EventManager::TriggerEvent(RemoveFromPlayerEquipmentEvent(m_ItemInStasis.iPlayerInventoryIndex, iSoldFor));
        }
        else {
            // Otherwise, trigger a RemoveFromPlayerInventoryEvent
            wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(m_ItemInStasis.pItem->GetName(), m_ItemInStasis.iPlayerInventoryIndex, iSoldFor));
        }
    }
    
    // We don't need to keep track of this information anymore so we reset it to the defaults
    m_ItemInStasis.pItem = nullptr;
    m_ItemInStasis.iPlayerInventoryIndex = -1;
}

void MerchantInventoryComponent::SellItemToPlayer(int p_iItemIndex, int p_iItemPrice) {
    // Retrieve the item at that index and send it to the player.
    // If they can afford it and fit it in their inventory, we'll get a "BuyItemFromMerchantEvent" in response    
    wolf::EventManager::TriggerEvent(SellItemToPlayerEvent(m_iIdNum, this->GetItem(p_iItemIndex), p_iItemPrice, p_iItemIndex));

    // Note that we send along the index we're storing the item at so that we can remove that specific instance
    // if the sale goes through, rather than just the first occurence of an item by that name, later
}

void MerchantInventoryComponent::ShowInventoryGUI() {
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // We want to show the merchant's name as part of the window title so we build a string with it real quick
        const std::string strTitle = " ~ " + m_strMerchantName + " ~";

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

        // Figure out where we'll be drawing the inventory
        ImVec2 v2DisplaySize = ImGui::GetIO().DisplaySize;
        ImVec2 v2WindowDrawPos = {v2DisplaySize.x - (7 * m_v2TexFrameSize.x), v2DisplaySize.y * 0.15f};

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(v2WindowDrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin(strTitle.c_str(), &m_bIsOpen, flags);

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
            m_bIsOpen = false;
        }

        ImGui::PopStyleColor(3);

        // Write the inventory title
        float fTextWidth = ImGui::CalcTextSize(strTitle.c_str()).x;

        ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
        ImGui::Text(strTitle.c_str());

        // If we closed the inventory using IMGUI
        if (!m_bIsOpen) {
            // Close it internally, too
            this->Close();
        }

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
                std::string strTooltipName;
                std::string strTooltipText = pItem->GetToolTipText();

                // Compute how much this merchant is selling the item for
                int iItemValue = pItem->GetValue();
                int iSalePrice = iItemValue + (iItemValue * m_fPercentMarkup);

                // And attach the price to the item's description
                strTooltipText += "\n\nPrice: " + std::to_string(iSalePrice);

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

                }
                else if (pItem->GetID() == EQUIPMENT) { // If this is an equipment item
                    // Try to cast it
                    EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
                    if (!pEquipment) {
                        // And throw an error if we couldn't
                        wolf::Error("Failed to cast ItemBase to EquipmentItem!\n");
                    }
                    
                    // Then construct the string that will be used to display all of the item's details
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

                // Draw the slot
                if (ImGui::ImageButton("Filled Slot", (void*)(intptr_t)m_pItemsTexture->GetID(), m_v2TexFrameSize, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2TopLeft, m_vv2ItemTextureCoords[pItem->GetTextureFrameIndex()]->m_v2BotRight)) {
                }

                // And pop the style vars and colors
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
                    if (ImGui::Button("Buy")) {
                        SellItemToPlayer(k, iSalePrice);
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

        // Text to show how much gold the merchant has
        ImGui::Text("  %s's Gold: %d", m_strMerchantName.c_str(), m_iGold);

        // New line for padding
        ImGui::NewLine();

        // End of window
        ImGui::End();

        // If the player is attempting to sell the merchant an item they cannot afford.
        if (m_bShowSellForLessPrompt) {
            // You can't resize the inventory or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // Push the message style vars and colors
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

            // Construct the text this prompt will display
            const std::string strMessage = m_ItemInStasis.pItem->GetName() + " is worth " + std::to_string(m_ItemInStasis.pItem->GetValue())
                + " Gold but " + m_strMerchantName + " only has " + std::to_string(m_iGold);

            // Figure out roughly how big the window will be as a result
            float fMessageWidth = ImGui::CalcTextSize(strMessage.c_str()).x;

            // Draw the message in the middle of the screen
            ImGui::SetNextWindowPos({v2DisplaySize.x * 0.5f - (fMessageWidth / 2.0f), v2DisplaySize.y * 0.45f});
            ImGui::SetNextWindowSize({0,0});
            ImGui::Begin("Not Enough Gold", nullptr, flags);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.239f, 0.239f, 0.239f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

            // Show a message asking the player if they are okay with selling the item for less than its value
            ImGui::Text(strMessage.c_str());
            
            // Calculate the size of the window and the text we want to display
            float fWindowWidth = ImGui::GetWindowWidth();
            float fTextWidth = ImGui::CalcTextSize("Sell Anyway?").x;

            // Set the cursor position and draw the text
            ImGui::SetCursorPosX((fWindowWidth - fTextWidth) * 0.5f);
            ImGui::Text("Sell Anyway?");
            ImGui::NewLine();

            // Calculate the size of the buttons
            float fButtonsText = ImGui::CalcTextSize(" Yes ").x + ImGui::CalcTextSize(" No ").x;

            // Reposition the cursor to line up the buttons
            ImGui::SetCursorPosX((fWindowWidth - fButtonsText) * 0.5f);
            
            // If they are
            if (ImGui::Button("Yes")) {
                // Close this prompt and process the sale
                m_bShowSellForLessPrompt = false;
                this->TakeItemOutOfStasis(true);
            }

            ImGui::SameLine();

            // If they aren't
            if (ImGui::Button("No")) {
                // Close the prompt and cancel the sale
                m_bShowSellForLessPrompt = false;
                this->TakeItemOutOfStasis(false);
            }

            // Pop the style vars and colors
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(4);

            ImGui::End();
        }
    }
}