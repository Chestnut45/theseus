#include "MerchantInventoryComponent.h"

#include <yaml-cpp/yaml.h>
#include "inventory/ItemCreator.h"

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
        // And another merchant just opened
        if (p_event.enType == MERCHANT_INVENTORY && p_event.iIdNum != m_iIdNum) {
            // Close this one
            m_bIsOpen = false;
        }
    }
}

void MerchantInventoryComponent::HandleCloseInventoryEvent(const CloseInventoryEvent& p_event) {
    // If the player just closed their inventory
    if (p_event.enType == PLAYER_INVENTORY) {
        // And this chest is open
        if (m_bIsOpen) {
            // Close it
            m_bIsOpen = false;
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

                // Then let the player know that the item has been sold
                wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(p_event.pItem->GetName(), p_event.iPlayerInventoryIndex, p_event.pItem->GetValue()));
            }
            else { // If the merchant does not have enough gold
                m_bShowSellForLessPrompt = true; // We need to ask the player if they are willing to sell the item, anyway

                // So we put the item that we're asking about in "stasis" so we can keep track of it outside of this method
                m_ItemInStasis.pItem = p_event.pItem;
                m_ItemInStasis.iPlayerInventoryIndex = p_event.iPlayerInventoryIndex;
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

        // Take away the gold we spent
        this->TakeGold(m_ItemInStasis.pItem->GetValue());

        // And let the player know the sale has gone through
        wolf::EventManager::TriggerEvent(RemoveFromPlayerInventoryEvent(m_ItemInStasis.pItem->GetName(), m_ItemInStasis.iPlayerInventoryIndex));
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
        // We want to show the merchant's name as part of the window title so we build a string with it real quick
        std::string strTitle = "  ~ " + m_strMerchantName + " ~";

        // You can't resize the inventory or move it
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos({800, 200});
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin(strTitle.c_str(), &m_bIsOpen, flags);

        // This counter lets us control how many items are drawn in a row
        int counter = 0;

        // We need to draw m_iSize number of slots
        for (int k = 0; k < m_iSize; k++) {
            // If there is an item (or stack of items as it were) in this slot
            if (!m_vvpContents[k].empty()) {
                // We grab a reference to the top item and create a variable to hold the item's details
                ItemBase* pItem = m_vvpContents[k].top();
                std::string strTooltipName;
                std::string strTooltipText;

                // Compute how much this merchant is selling the item for
                int iItemValue = pItem->GetValue();
                int iSalePrice = iItemValue + (iItemValue * m_fPercentMarkup);

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
                    strTooltipText = pConsumable->GetDescription() + "\nUses: " + std::to_string(pConsumable->GetNumUses()) + "\n\nPrice: " + std::to_string(iSalePrice);

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
                    strTooltipText = pEquipment->GetDescription() + "\nSlot: " + pEquipment->GetEquipmentSlotString()
                        + "\n\nPrice: " + std::to_string(iSalePrice);
                }
                else { // If for some reason this item isn't Consumable OR Equipment
                    strTooltipName = pItem->GetName() + " (" + std::to_string(m_vvpContents[k].size()) + ")";
                    strTooltipText = pItem->GetDescription() + "\n\nPrice: " + std::to_string(iSalePrice);
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
                    RGBIntColor nameColor = RarityColors[pItem->GetRarity()];
                    ImGui::TextColored(ImColor(nameColor.r, nameColor.g, nameColor.b), "%s", strTooltipName.c_str());
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

        ImGui::Text("%s's Gold: %d", m_strMerchantName.c_str(), m_iGold);

        // End of window
        ImGui::End();

        // If the player is attempting to sell the merchant an item they cannot afford.
        if (m_bShowSellForLessPrompt) {
            // You can't resize the inventory or move it
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

            // By default, the prompt appears close to the middle of the screen
            ImGui::SetNextWindowPos({550, 300});
            ImGui::SetNextWindowSize({0,0});
            ImGui::Begin("Not Enough Gold", nullptr, flags);

            // Show a message asking the player if they are okay with selling the item for less than its value
            ImGui::Text("%s is worth %d Gold but %s only has %d.", m_ItemInStasis.pItem->GetName().c_str(), m_ItemInStasis.pItem->GetValue(), m_strMerchantName.c_str(), m_iGold);
            ImGui::Text("\t\t\t\t\tSell Anyway?");
            ImGui::NewLine();
            ImGui::Text("\t\t\t\t\t");
            ImGui::SameLine();
            
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

            ImGui::End();
        }
    }
}