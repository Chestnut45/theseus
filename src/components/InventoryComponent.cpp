#include "InventoryComponent.h"
#include "W_Logging.h"

#include <yaml-cpp/yaml.h>
#include "../inventory/ItemCreator.h"

int InventoryComponent::m_iNextIdNum = 0;

const float InventoryComponent::TOOLTIP_WRAP_POS = 176.0f;

// Shared texture resources
const std::string InventoryComponent::m_strItemsTexturePath = "data/textures/ItemIcons-Sheet.png";
const ImVec2 InventoryComponent::m_v2TexFrameSize = {32.0f, 32.0f};
const int InventoryComponent::m_iEmptySlotIndex = 25;
std::vector<ImGuiUVSet*> InventoryComponent::m_vv2ItemTextureCoords;

const std::string InventoryComponent::m_strFrameTexturePath = "data/textures/InventoryUI.png";
std::vector<ImGuiUVSet*> InventoryComponent::m_vv2FrameTextureCoords;

InventoryComponent::InventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos) : m_iSize(p_iSize), m_iMaxPerRow(p_iSlotsPerRow), m_iIdNum(m_iNextIdNum), m_v2DrawPos(p_v2DrawPos){
    // Reserve the amount of space we've been asked for
    m_vvpContents.reserve(p_iSize);

    // And fill it up with empty stacks
    for (int i = 0; i < p_iSize; i++) {
        std::stack<ItemBase*> stack;
        m_vvpContents.push_back(stack);
    }

    // If this is the first InventoryComponent that is created
    if (!m_pItemsTexture) {
        m_pItemsTexture = this->InitTexture(m_strItemsTexturePath, m_vv2ItemTextureCoords);
    }

    if (!m_pFrameTexture) {
        m_pFrameTexture = this->InitTexture(m_strFrameTexturePath, m_vv2FrameTextureCoords);
    }

    m_iNextIdNum++;
}

wolf::Texture* InventoryComponent::InitTexture(const std::string& p_strTexturePath, std::vector<ImGuiUVSet*>& p_vv2TextureCoords) {
    // We need to initalize the shared textures
    wolf::Texture* pNewTexture = wolf::TextureManager::CreateTexture(p_strTexturePath);
    if (pNewTexture) {
        if ((pNewTexture->GetWidth() * pNewTexture->GetHeight()) % (int)(m_v2TexFrameSize.x * m_v2TexFrameSize.y) != 0) {
            // If the new texture doesn't match the frame size then we delete it and leave the current texture unchanged
            wolf::TextureManager::DestroyTexture(pNewTexture);
            wolf::Error("Incorrectly sized texture file \"", p_strTexturePath, "\" passed to InventoryComponent.");
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
        p_vv2TextureCoords.push_back(pOffsetCoord);

        // Now that we have all our UV coordinates, we're going to assign them to frames
        for (int p = 0; p <= iNumFramesY - 1; p++) {
            for (int q = 0; q <= iNumFramesX - 1; q++) {
                int iOriginPoint = (p * iWidth) + q;

                // First we find the four UV coordinates that will be used to render this frame
                // and store them in a struct that holds four glm::vec2s
                ImGuiUVSet* pTexFrameCords = new ImGuiUVSet(av2WorkingUVCoords[iOriginPoint], av2WorkingUVCoords[iOriginPoint + iWidth + 1]);

                // Then we store 'em
                p_vv2TextureCoords.push_back(pTexFrameCords);
            }
        }

        pNewTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest);
        return pNewTexture;
    }

    return nullptr;
}

InventoryComponent::~InventoryComponent() {
    // Empty each of the stacks in the contents vector
    this->EmptyInventory();

    // Then delete the contents vector itself
    m_vvpContents.clear();
}

ItemBase* InventoryComponent::GetItem(const std::string& p_strItemName) {
    // If there are no slots in use then we can't return anything!
    if (m_iSlotsInUse == 0) {
        return nullptr;
    }

    // Look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same name as what we're looking for
        if (strcmp(pItem->GetName().c_str(), p_strItemName.c_str()) == 0) {
            return pItem; // We return it!
        }
    }

    // Otherwise, return a null pointer
    return nullptr;
}

// Follows the same process as GetItem with the item name as the parameter
ItemBase* InventoryComponent::GetItem(ItemID p_enItemID) {
    // If there are no slots in use then we can't return anything!
    if (m_iSlotsInUse == 0) {
        return nullptr;
    }

    // Look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same name as what we're looking for
        if (pItem->GetID() == p_enItemID) {
            return pItem; // We return it!
        }
    }

    // Otherwise, return a null pointer
    return nullptr;
}

// This GetItem method is for when we know the index (inventory "slot" number) of the item we're looking for
ItemBase* InventoryComponent::GetItem(int p_iItemIndex) {
    if (p_iItemIndex > m_vvpContents.size() || p_iItemIndex < 0 || p_iItemIndex >= m_iSlotsInUse) {
        // That's an invalid index so we can't get the item there
        return nullptr;
    }

    // By default we return the first item in a "stack" of items
    return m_vvpContents[p_iItemIndex].top();
}

bool InventoryComponent::AddItem(ItemBase* p_pItem) {
    // If we're not using any of the slots then we can just insert the item
    if (m_iSlotsInUse == 0) {
        m_vvpContents[m_iSlotsInUse].push(p_pItem);
        m_iSlotsInUse++;
        return true;
    }

    // Otherwise we need to look through the inventory contents
    for (int i = 0; i < m_iSlotsInUse; i++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[i].top();

        // If we find an item with the same ID and it is stackable
        if (pItem->GetID() == p_pItem->GetID() && p_pItem->IsStackable() && pItem->IsStackable()) {
            // Then we push the item to the stack
            m_vvpContents[i].push(p_pItem);
            m_iLastUsedSlot = i; // Save what index we added the item to
            return true;
        }
    }

    // Otherwise, we check if there is an empty inventory slot to start a new item stack in
    if (m_iSlotsInUse < m_iSize) {
        // Push the item to the next open slot
        m_vvpContents[m_iSlotsInUse].push(p_pItem);
        m_iLastUsedSlot = m_iSlotsInUse; // Save what index we added the item to

        // Update the number of slots we're using
        m_iSlotsInUse++;

        // And return true
        return true;
    }

    // And if all that fails, we keep track of the failed attempt and return false
    m_iLastUsedSlot = -1;
    return false;
}

// Note that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(const std::string& p_strItemName) {
    // If our inventory is empty then we can't remove anything!
    if (m_iSlotsInUse == 0) {
        return false;
    }

    for (int p = 0; p < m_iSlotsInUse; p++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[p].top();

        // If we find an item with the same name as what we're looking for
        if (strcmp(pItem->GetName().c_str(), p_strItemName.c_str()) == 0) {
            // Then we remove it
            m_vvpContents[p].pop();

            // And check if that stack is now empty
            if (m_vvpContents[p].empty()) {
                // If it is, then we need to decrease the number of slots in use
                m_iSlotsInUse--;

                // And reorganize the vector to fill the gap by removing the empty stack from the vector
                std::stack<ItemBase*> pEmptyStack = m_vvpContents[p];
                m_vvpContents.erase(m_vvpContents.begin() + p);

                // And then pushing it back onto the vector
                m_vvpContents.push_back(pEmptyStack);
            }

            // And return true
            return true;
        }
    }

    // Otherwise, we couldn't remove the item so we return false
    return false;
}

// Once again, note that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(ItemID p_enItemID) {
    // If our inventory is empty then we can't remove anything!
    if (m_iSlotsInUse == 0) {
        return false;
    }

    for (int p = 0; p < m_iSlotsInUse; p++) {
        // Inventory "slots" are stacks whose items are all the same so we can just look at the
        // first item in them when we're looking for something
        ItemBase* pItem = m_vvpContents[p].top();

        // If we find an item with the same ID as what we're looking for
        if (pItem->GetID() == p_enItemID) {
            // Then we remove it
            m_vvpContents[p].pop();

            // And check if that stack is now empty
            if (m_vvpContents[p].empty()) {
                // If it is, then we need to decrease the number of slots in use
                m_iSlotsInUse--;

                // And reorganize the vector to fill the gap by removing the empty stack from the vector
                std::stack<ItemBase*> pEmptyStack = m_vvpContents[p];
                m_vvpContents.erase(m_vvpContents.begin() + p);

                // And then pushing it back onto the vector
                m_vvpContents.push_back(pEmptyStack);
                
            }

            // And return true
            return true;
        }
    }

    // Otherwise, we couldn't remove the item so we return false
    return false;
}

// Keep in mind that removing an item from the inventory DOES NOT DELETE IT.
bool InventoryComponent::RemoveItem(int p_iItemIndex) {
    if (p_iItemIndex > m_vvpContents.size() || p_iItemIndex < 0 || p_iItemIndex >= m_iSlotsInUse) {
        // If we're given an invalid index then we return false
        return false;
    }

    // Retrieve the item that we're trying to delete
    ItemBase* pItem = m_vvpContents[p_iItemIndex].top();

    // If it is a piece of equipment
    if (pItem->GetID() == EQUIPMENT) {
        // Unequip it before we delete it
        EquipmentItem* pEquipment = dynamic_cast<EquipmentItem*>(pItem);
        pEquipment->SetEquipped(false);
    }

    // Otherwise we remove the first item in the stack at the given index
    m_vvpContents[p_iItemIndex].pop();

    // And check if doing so emptied that stack
    if (m_vvpContents[p_iItemIndex].empty()) {
        // If so, we decrease the number of slots in use
        m_iSlotsInUse--;

        // And reorganize the vector to fill the gap by removing the empty stack from the vector
        std::stack<ItemBase*> pEmptyStack = m_vvpContents[p_iItemIndex];
        m_vvpContents.erase(m_vvpContents.begin() + p_iItemIndex);

        // And then pushing it back onto the vector
        m_vvpContents.push_back(pEmptyStack);
    }

    // And return true
    return true;
}

void InventoryComponent::EmptyInventory() {
    // Go through the contents vector
    for (std::vector<std::stack<ItemBase*>>::iterator it = m_vvpContents.begin(); it != m_vvpContents.end(); ++it) {
        // And empty each of the item stacks within
        while (!it->empty()) {
            ItemBase* pNextItem = it->top();
            delete pNextItem;
            it->pop();
        }
    }

    // Our inventory is empty now so we're not using any of the slots
    m_iSlotsInUse = 0;
}

void InventoryComponent::ShowInventoryGUI() {  
    if (m_bIsOpen) {
        ImGuiStyle* pStyle = &ImGui::GetStyle();
        pStyle->WindowTitleAlign = ImVec2(0.5f, 0.5f);

        // You can't resize the inventory but you can move it around!
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

        // By default, the inventory appears close to the middle of the screen
        ImGui::SetNextWindowPos(m_v2DrawPos);
        ImGui::SetNextWindowSize({0,0});
        ImGui::Begin("~ Inventory ~", nullptr, flags);

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
                    strTooltipText = pEquipment->GetDescription() + "\n\nValue: " + std::to_string(pEquipment->GetValue()) + "\nSlot: " + pEquipment->GetEquipmentSlotString();
                }
                else { // If for some reason this item isn't Consumable OR Equipment
                    strTooltipName = pItem->GetName();
                    strTooltipText = pItem->GetDescription(); // We only show the name and the description
                }
                
                // We're also going to store a string representation of the slot index that we're on
                // so that we can create unique tooltips for each slot later
                std::string strIndex = std::to_string(k);

                // Now we can start making the actual buttons
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
                }
            }
            else { // Otherwise, this is an empty inventory slot
            if (ImGui::ImageButton("Empty Slot", (void*)(intptr_t)m_pFrameTexture->GetID(), m_v2TexFrameSize, m_vv2FrameTextureCoords[m_iEmptySlotIndex]->m_v2TopLeft, m_vv2FrameTextureCoords[m_iEmptySlotIndex]->m_v2BotRight)) {

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


bool InventoryComponent::FillInventoryFromFile(const std::string& p_strFilePath) {
    try {
        // Load the file
        YAML::Node node = YAML::LoadFile(p_strFilePath);

        // Go through the list of items
        YAML::Node itemList = node["item_list"];
        for (int i = 0; i < itemList.size(); ++i) {
            std::string strItemName = itemList[i].as<std::string>();
            
            // Try to create one
            ItemBase* pNextItem = ItemCreator::CreateItem(strItemName);

            // If it works,
            if (pNextItem) {

                // Add it to the inventory
                this->AddItemOrDelete(pNextItem);
            }
            else {
                // Otherwise return false
                return false;
            }
        }
    }
    catch (YAML::Exception& e) {
        // If we run into an error, then we should print it and return false
        wolf::Error("Error using '", p_strFilePath.c_str(), ": ", e.what());
        return false;
    }

    // If we didn't encounter any issues, we return true
    return true;
}

void InventoryComponent::Open() {
    m_bIsOpen = true;

    // Let anyone interested know which specific chest was opened
    wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
}

void InventoryComponent::Close() {
    m_bIsOpen = false;

    // Let anyone interested know which specific chest was closed
    wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
}

void InventoryComponent::ToggleOpen() {
    m_bIsOpen = !m_bIsOpen;

    if (m_bIsOpen) {
        wolf::EventManager::TriggerEvent(OpenInventoryEvent(m_enType, m_iIdNum));
    }
    else {
        wolf::EventManager::TriggerEvent(CloseInventoryEvent(m_enType, m_iIdNum));
    }
}