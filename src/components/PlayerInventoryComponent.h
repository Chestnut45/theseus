#pragma once

//-----------------------------------------------------------------------------
// File:            PlayerInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing the player's inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"
#include "events/DroppedItemEvents.h"
#include "inventory/ItemDropCreator.h"

#include "inventory/FlatAmtItem.h"
#include "inventory/PercentItem.h"
#include "inventory/StatusEffectItem.h"

#include "inventory/ArmourItem.h"
#include "inventory/WeaponItem.h"

class PlayerInventoryComponent : public InventoryComponent {
    public:
        PlayerInventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_v2DrawPos)
            {
                m_enType = PLAYER_INVENTORY;

                // We need to fill the equipment array with invalid indices so we don't
                // automatically equip the first item added to the inventory
                for (int i = 0; i < END_OF_EQUIPMENT; i++) {
                    m_iEquipmentSlots[i] = -1;
                }

                // We also need to fill the schematics array with zeros because we don't have any schematics, yet
                for (int j = 0; j < END_OF_RARITIES; j++) {
                    m_iSchematics[j] = 0;
                }

                // Start with exactly one common schematic
                m_iSchematics[0] = 1;

                // We also need to register for events related to the player's inventory
                wolf::EventManager::AddListener<OpenInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleOpenInventoryEvent>(*this);
                wolf::EventManager::AddListener<CloseInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleCloseInventoryEvent>(*this);
                wolf::EventManager::AddListener<SellItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleSellItemToPlayerEvent>(*this);
                wolf::EventManager::AddListener<PickupDroppedItemEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandlePickupDroppedItemEvent>(*this);
                wolf::EventManager::AddListener<DispenseItemToPlayerEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleDispenseItemToPlayerEvent>(*this);
                wolf::EventManager::AddListener<SendItemToPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleAddToPlayerInventoryEvent>(*this);
                wolf::EventManager::AddListener<RemoveFromPlayerInventoryEvent, PlayerInventoryComponent, &PlayerInventoryComponent::HandleRemoveFromPlayerInventoryEvent>(*this);
            };
        
        ~PlayerInventoryComponent();

        // Delete copy constructor/assignment
        PlayerInventoryComponent(const PlayerInventoryComponent&) = delete;
        PlayerInventoryComponent& operator=(const PlayerInventoryComponent&) = delete;

        // Delete move constructor/assignment
        PlayerInventoryComponent(PlayerInventoryComponent&& other) = delete;
        PlayerInventoryComponent& operator=(PlayerInventoryComponent&& other) = delete;

        ItemBase* GetEquippedItem(EquipmentSlot p_enSlot);

        virtual void Close();

        virtual void ShowInventoryGUI();

        void AddGold(int p_iAmt);
        bool TakeGold(int p_iAmt);

        inline int GetGold() {return m_iGold;};

        bool TakeSchematic(Rarity p_enRarity);
        void AddSchematic(Rarity p_enRarity);
        int GetNumSchematics();

        inline int GetNumSchematicsOfRarity(Rarity p_enRarity) const {return m_iSchematics[p_enRarity];};

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleCloseInventoryEvent(const CloseInventoryEvent& p_event);
        void HandleSellItemToPlayerEvent(const SellItemToPlayerEvent& p_event);
        void HandlePickupDroppedItemEvent(const PickupDroppedItemEvent& p_event);
        void HandleDispenseItemToPlayerEvent(const DispenseItemToPlayerEvent& p_event);
        void HandleAddToPlayerInventoryEvent(const SendItemToPlayerInventoryEvent& p_event);
        void HandleRemoveFromPlayerInventoryEvent(const RemoveFromPlayerInventoryEvent& p_event);

    private:
        void UseItem(ItemBase* p_pItem, int p_iItemIndex);
        void EquipItem(ItemBase* p_pItem, int p_iItemIndex);
        void UnequipItem(ItemBase* p_pItem);
        void DiscardItem(int p_iItemIndex);

        bool m_bShowFullInventoryPrompt = false;
        bool m_bShowTooExpensivePrompt = false;
        bool m_bShowMissingSchematicPrompt = false;

        const int MAX_SCHEMATICS_PER_RARITY = 99;

        const int MAX_GOLD = 999;
        int m_iGold = 0;

        int m_iOpenChestIdNum = -1;
        int m_iOpenMerchantIdNum = -1;

        int m_iEquipmentSlots[END_OF_EQUIPMENT];
        int m_iSchematics[END_OF_RARITIES];
};