#pragma once

//-----------------------------------------------------------------------------
// File:            DispensaryInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing a armament dispensary inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"

class DispensaryInventoryComponent : public InventoryComponent {
    public:
        DispensaryInventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_v2DrawPos)
            {
                m_enType = DISPENSARY_INVENTORY;

                wolf::EventManager::AddListener<OpenInventoryEvent, DispensaryInventoryComponent, &DispensaryInventoryComponent::HandleOpenInventoryEvent>(*this);
                wolf::EventManager::AddListener<CloseInventoryEvent, DispensaryInventoryComponent, &DispensaryInventoryComponent::HandleCloseInventoryEvent>(*this);
            };

        ~DispensaryInventoryComponent();

        // Delete copy constructor/assignment
        DispensaryInventoryComponent(const DispensaryInventoryComponent&) = delete;
        DispensaryInventoryComponent& operator=(const DispensaryInventoryComponent&) = delete;

        // Delete move constructor/assignment
        DispensaryInventoryComponent(DispensaryInventoryComponent&& other) = delete;
        DispensaryInventoryComponent& operator=(DispensaryInventoryComponent&& other) = delete;

        // Dispensary inventories behave a bit differently than other inventories
        // so we need to overload quite a bit of the base functionality

        virtual bool FillInventoryFromFile(const std::string& p_strFilePath);

        virtual bool AddItem(ItemBase* p_pItem);
        
        virtual bool RemoveItem(const std::string& p_strItemName);
        virtual bool RemoveItem(ItemID p_enItemID);
        virtual bool RemoveItem(int p_iItemIndex);

        virtual void EmptyInventory();

        virtual void ShowInventoryGUI();

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleCloseInventoryEvent(const CloseInventoryEvent& p_event);

    private:
        void DispenseItem(int p_iItemIndex);
        void SortByRarity();

        bool m_bUnsorted = true;

        std::vector<int> m_arContentsByRarity[END_OF_RARITIES];
};