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
#include "inventory/ItemBase.h"

class InventoryComponent : public wolf::BaseComponent {
    public:
        InventoryComponent(int p_iSize, int p_iSlotsPerRow);
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

        void DEBUGPrintInventory();

    private:
        const int m_iSize;
        const int m_iMaxPerRow;
        int m_iSlotsInUse = 0;

        std::vector<std::stack<ItemBase*>> m_vvpContents;
};