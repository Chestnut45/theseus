#pragma once

//-----------------------------------------------------------------------------
// File:            InventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing a consumable item which causes Theseus to incur a given
// status effect
//-----------------------------------------------------------------------------

#include <wolf.h>
#include <vector>
#include <stack>
#include <imgui/imgui.h>

#include "inventory/ItemBase.h"
#include "inventory/ConsumableItem.h"
#include "inventory/EquipmentItem.h"

struct ImGuiUVSet {
    ImGuiUVSet(ImVec2 p_v2TopLeft, ImVec2 p_v2BotRight) : m_v2TopLeft(p_v2TopLeft), m_v2BotRight(p_v2BotRight) {};
    ImVec2 m_v2TopLeft;
    ImVec2 m_v2BotRight;
};

class InventoryComponent : public wolf::BaseComponent {
    public:
        InventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize);
        ~InventoryComponent();

        ItemBase* GetItem(const std::string& p_strItemName);
        ItemBase* GetItem(ItemID p_enItemID);
        ItemBase* GetItem(int p_iItemIndex);

        bool AddItem(ItemBase* p_pItem);

        bool RemoveItem(const std::string& p_strItemName);
        bool RemoveItem(ItemID p_enItemID);
        bool RemoveItem(int p_iItemIndex);

        void SortByName();
        void SortByValue();
        void SortByType();

        void EmptyInventory();

        void ShowInventoryGUI();

    private:
        void UseItem(ItemBase* p_pItem, int p_iItemIndex);
        void EquipItem(ItemBase* p_pItem);
        void UnequipItem(ItemBase* p_pItem);
        void DiscardItem(int p_iItemIndex);

        const int m_iSize;
        const int m_iMaxPerRow;
        int m_iSlotsInUse = 0;

        std::vector<std::stack<ItemBase*>> m_vvpContents;
        std::vector<ImGuiUVSet*> m_vv2TextureCoords;

        wolf::Texture* m_pTexture;
        ImVec2 m_v2TexFrameSize;
};