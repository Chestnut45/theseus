#pragma once

//-----------------------------------------------------------------------------
// File:            InventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing a basic inventory
//-----------------------------------------------------------------------------

#include <wolf.h>
#include <vector>
#include <stack>
#include <imgui/imgui.h>

#include "inventory/ItemBase.h"
#include "inventory/ConsumableItem.h"
#include "inventory/EquipmentItem.h"

// Note that this struct is NOT a part of the ImGui library it just uses ImVec2s
struct ImGuiUVSet {
    ImGuiUVSet(ImVec2 p_v2TopLeft, ImVec2 p_v2BotRight) : m_v2TopLeft(p_v2TopLeft), m_v2BotRight(p_v2BotRight) {};
    ImVec2 m_v2TopLeft;
    ImVec2 m_v2BotRight;
};

class InventoryComponent : public wolf::BaseComponent {
    public:
        InventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize);
        ~InventoryComponent();

        // Delete copy constructor/assignment
        InventoryComponent(const InventoryComponent&) = delete;
        InventoryComponent& operator=(const InventoryComponent&) = delete;

        // Delete move constructor/assignment
        InventoryComponent(InventoryComponent&& other) = delete;
        InventoryComponent& operator=(InventoryComponent&& other) = delete;

        ItemBase* GetItem(const std::string& p_strItemName);
        ItemBase* GetItem(ItemID p_enItemID);
        ItemBase* GetItem(int p_iItemIndex);

        bool AddItem(ItemBase* p_pItem);

        bool RemoveItem(const std::string& p_strItemName);
        bool RemoveItem(ItemID p_enItemID);
        bool RemoveItem(int p_iItemIndex);

        void EmptyInventory();
        virtual void ShowInventoryGUI();

    protected:
        const int m_iSize;
        const int m_iMaxPerRow;
        int m_iSlotsInUse = 0;

        std::vector<std::stack<ItemBase*>> m_vvpContents;
        std::vector<ImGuiUVSet*> m_vv2TextureCoords;

        wolf::Texture* m_pTexture;
        ImVec2 m_v2TexFrameSize;
};