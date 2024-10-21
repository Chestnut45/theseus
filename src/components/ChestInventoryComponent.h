#pragma once

//-----------------------------------------------------------------------------
// File:            ChestInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing a given chest's inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"

class ChestInventoryComponent : public InventoryComponent {
    public:
        ChestInventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_strTexture, p_v2TexFrameSize)
            {
                m_enType = CHEST_INVENTORY;

                wolf::EventManager::AddListener<SendItemToChestEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleAddToChestEvent>(*this);
                wolf::EventManager::AddListener<RemoveFromChestEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleRemoveFromChestEvent>(*this);
                wolf::EventManager::AddListener<OpenInventoryEvent, ChestInventoryComponent, &ChestInventoryComponent::HandleOpenInventoryEvent>(*this);
            };

        ~ChestInventoryComponent();

        bool FillChestFromFile(const std::string& p_strFilePath);
        bool IsOpen() {return m_bIsOpen;};

        void OpenChest();
        void CloseChest();

        void HandleAddToChestEvent(const SendItemToChestEvent& p_event);
        void HandleRemoveFromChestEvent(const RemoveFromChestEvent& p_event);
        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);

        virtual void ShowInventoryGUI();

    private:
        void SendItemToPlayer(int p_iItemIndex);
        bool m_bIsOpen = false;
};