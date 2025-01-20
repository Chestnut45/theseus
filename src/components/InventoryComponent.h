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
    MERCHANT_INVENTORY,
    DISPENSARY_INVENTORY
};

// Note that this struct is NOT a part of the ImGui library it just uses ImVec2s
struct ImGuiUVSet {
    ImGuiUVSet(ImVec2 p_v2TopLeft, ImVec2 p_v2BotRight) : m_v2TopLeft(p_v2TopLeft), m_v2BotRight(p_v2BotRight) {};
    ImVec2 m_v2TopLeft;
    ImVec2 m_v2BotRight;
};

class InventoryComponent : public wolf::BaseComponent {
    public:
        InventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos);
        ~InventoryComponent();

        // Delete copy constructor/assignment
        InventoryComponent(const InventoryComponent&) = delete;
        InventoryComponent& operator=(const InventoryComponent&) = delete;

        // Delete move constructor/assignment
        InventoryComponent(InventoryComponent&& other) = delete;
        InventoryComponent& operator=(InventoryComponent&& other) = delete;

        virtual bool FillInventoryFromFile(const std::string& p_strFilePath);

        InventoryType GetType() {return m_enType;};
        int GetIdNum() {return m_iIdNum;};

        ItemBase* GetItem(const std::string& p_strItemName);
        ItemBase* GetItem(ItemID p_enItemID);
        ItemBase* GetItem(int p_iItemIndex);

        virtual void Open();
        virtual void Close();
        virtual void ToggleOpen();

        bool IsOpen() const {return m_bIsOpen;};
        bool IsEmpty() const {return m_iSlotsInUse == 0;};

        virtual bool AddItem(ItemBase* p_pItem);

        int GetLastUsedSlot() const {return m_iLastUsedSlot;};

        // Safe wrapper to delete items if they could not be added
        bool AddItemOrDelete(ItemBase* p_pItem)
        {
            bool success = AddItem(p_pItem);
            if (!success) delete p_pItem;
            return success; 
        }

        virtual bool RemoveItem(const std::string& p_strItemName);
        virtual bool RemoveItem(ItemID p_enItemID);
        virtual bool RemoveItem(int p_iItemIndex);

        virtual void EmptyInventory();
        virtual void ShowInventoryGUI();

        void SetDrawPosition(ImVec2 p_v2Pos) {m_v2DrawPos = p_v2Pos;};
        ImVec2 GetDrawPosition() const {return m_v2DrawPos;};

    protected:
        static const float TOOLTIP_WRAP_POS;

        static int m_iNextIdNum;
        const int m_iIdNum;

        const int m_iSize;
        const int m_iMaxPerRow;
        int m_iSlotsInUse = 0;
        int m_iLastUsedSlot = 0;

        bool m_bIsOpen = false;

        ImVec2 m_v2DrawPos = {0.0f, 0.0f};

        InventoryType m_enType = BASIC_INVENTORY;

        std::vector<std::stack<ItemBase*>> m_vvpContents;

        // Shared texture resources for Items
        static std::vector<ImGuiUVSet*> m_vv2ItemTextureCoords;
        static const int m_iEmptySlotIndex;
        static const std::string m_strItemsTexturePath;
        static const ImVec2 m_v2TexFrameSize;
        static inline wolf::Texture* m_pItemsTexture = nullptr;
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

    // This flag should be used anytime we are attempting to store an equipped item
    bool bWasEquipped = false;
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

struct SellItemToPlayerEvent {
    int iMerchantIdNum;

    ItemBase* pItem;
    int iPrice;

    // This is an optional index that should be included whenever possible
    int iMerchantInventoryIndex = -1;
};

struct SellItemToMerchantEvent {
    int iMerchantIdNum;
    ItemBase* pItem;

    // This is an optional index that should be included whenever possible
    int iPlayerInventoryIndex;

    // This flag should be used anytime we are attempting to store an equipped item
    bool bWasEquipped = false;
};

struct BoughtItemFromMerchantEvent {
    int iMerchantIdNum;
    std::string strItemName;

    int iBoughtFor;

    // This is also an optional index that should be included whenever possible
    int iMerchantInventoryIndex = -1;
};

struct DispenseItemToPlayerEvent {
    int iDispensaryIdNum;
    ItemBase* pItem;
};