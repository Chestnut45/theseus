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

enum InventoryType {
    BASIC_INVENTORY,
    CHEST_INVENTORY,
    PLAYER_INVENTORY,
    MERCHANT_INVENTORY
};

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

        InventoryType GetType() {return m_enType;};
        int GetIdNum() {return m_iIdNum;};

        ItemBase* GetItem(const std::string& p_strItemName);
        ItemBase* GetItem(ItemID p_enItemID);
        ItemBase* GetItem(int p_iItemIndex);

        virtual void Open() {m_bIsOpen = true;};
        virtual void Close() {m_bIsOpen = false;};
        virtual void ToggleOpen() {m_bIsOpen = !m_bIsOpen;};

        bool IsOpen() {return m_bIsOpen;};

        bool AddItem(ItemBase* p_pItem);

        bool RemoveItem(const std::string& p_strItemName);
        bool RemoveItem(ItemID p_enItemID);
        bool RemoveItem(int p_iItemIndex);

        void EmptyInventory();
        virtual void ShowInventoryGUI();

    protected:
        static int m_iNextIdNum;
        const int m_iIdNum;

        const int m_iSize;
        const int m_iMaxPerRow;
        int m_iSlotsInUse = 0;

        bool m_bIsOpen = false;

        InventoryType m_enType = BASIC_INVENTORY;

        std::vector<std::stack<ItemBase*>> m_vvpContents;
        std::vector<ImGuiUVSet*> m_vv2TextureCoords;

        wolf::Texture* m_pTexture;
        ImVec2 m_v2TexFrameSize;
};

struct OpenInventoryEvent {
    InventoryType enType;
    int iIdNum;
};

struct CloseInventoryEvent {
    InventoryType enType;
    int iIdNum;
};

struct SendItemToChestEvent {
    int iChestIdNum;
    ItemBase* pItem;

    // It is technically optional to include the item's player inventory index, but
    // it should be used whenever possible to make sure that we remove a
    // specific item instance rather than the first one we find.
    int iPlayerInventoryIndex;
};

struct RemoveFromChestEvent {
    int iChestIdNum;
    std::string strItemName;

    // This is a similarly optional index that should be included whenever possible
    int iChestInventoryIndex = -1;
};

struct SendItemToPlayerInventoryEvent {
    InventoryType enSenderType;
    int iSenderIdNum;

    ItemBase* pItem;

    // This is also an optional index that should be included whenever possible
    int iSenderInventoryIndex = -1;
};