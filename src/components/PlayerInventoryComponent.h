#pragma once

//-----------------------------------------------------------------------------
// File:            PlayerInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing the player's inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"

class PlayerInventoryComponent : public InventoryComponent {
    public:
        PlayerInventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_strTexture, p_v2TexFrameSize)
            {
                m_enType = PLAYER_INVENTORY;

                // We need to fill the equipment array with invalid indices so we don't
                // automatically equip the first item added to the inventory
                for (int i = 0; i < END_OF_EQUIPMENT; i++) {
                    m_iEquipmentSlots[i] = -1;
                }

                // We also need to register for events related to the player's inventory
                wolf::EventManager::AddListener<OpenInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleOpenInventoryEvent>(*this);
                wolf::EventManager::AddListener<AddToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
                wolf::EventManager::AddListener<DeleteFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDeleteFromPlayerInventoryEvent>(*this);
            };
        
        ~PlayerInventoryComponent();

        // Delete copy constructor/assignment
        PlayerInventoryComponent(const PlayerInventoryComponent&) = delete;
        PlayerInventoryComponent& operator=(const PlayerInventoryComponent&) = delete;

        // Delete move constructor/assignment
        PlayerInventoryComponent(PlayerInventoryComponent&& other) = delete;
        PlayerInventoryComponent& operator=(PlayerInventoryComponent&& other) = delete;

        ItemBase* GetEquippedItem(EquipmentSlot p_enSlot);

        virtual void ShowInventoryGUI();

        void AddGold(int p_iAmt);
        bool TakeGold(int p_iAmt);

        inline int GetGold() {return m_iGold;};

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleAddToPlayerInventoryEvent(const AddToPlayerInventoryEvent& p_event);
        void HandleDeleteFromPlayerInventoryEvent(const DeleteFromPlayerInventoryEvent& p_event);

    private:
        void UseItem(ItemBase* p_pItem, int p_iItemIndex);
        void EquipItem(ItemBase* p_pItem, int p_iItemIndex);
        void UnequipItem(ItemBase* p_pItem);
        void DiscardItem(int p_iItemIndex);

        const int MAX_GOLD = 999;
        int m_iGold = 0;

        int m_iEquipmentSlots[END_OF_EQUIPMENT - 1];
};