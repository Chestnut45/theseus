#pragma once

//-----------------------------------------------------------------------------
// File:            PlayerInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing the player's inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"
#include "events/DroppedItemEvents.h"
#include "events/InventoryEvents.h"
#include "inventory/ItemDropCreator.h"

#include "inventory/FlatAmtItem.h"
#include "inventory/PercentItem.h"
#include "inventory/StatusEffectItem.h"

#include "inventory/ArmourItem.h"
#include "inventory/WeaponItem.h"

class PlayerInventoryComponent : public InventoryComponent {
    public:
        PlayerInventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos);
        ~PlayerInventoryComponent();

        // Delete copy constructor/assignment
        PlayerInventoryComponent(const PlayerInventoryComponent&) = delete;
        PlayerInventoryComponent& operator=(const PlayerInventoryComponent&) = delete;

        // Delete move constructor/assignment
        PlayerInventoryComponent(PlayerInventoryComponent&& other) = delete;
        PlayerInventoryComponent& operator=(PlayerInventoryComponent&& other) = delete;

        ItemBase* GetEquippedItem(EquipmentSlot p_enSlot);
        void RemoveEquippedItem(EquipmentSlot p_enSlot);

        virtual void ShowInventoryGUI();
        void ShowToggleButtonGUI();

        void AddGold(int p_iAmt);
        bool TakeGold(int p_iAmt);

        inline int GetGold() {return m_iGold;};

        bool TakeSchematic(Rarity p_enRarity);
        void AddSchematic(Rarity p_enRarity);
        int GetNumSchematics();

        inline bool IsToggleButtonHovered() { return m_bToggleButtonHovered; }

        inline int GetNumSchematicsOfRarity(Rarity p_enRarity) const {return m_iSchematics[p_enRarity];};

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleCloseInventoryEvent(const CloseInventoryEvent& p_event);
        void HandleSellItemToPlayerEvent(const SellItemToPlayerEvent& p_event);
        void HandlePickupDroppedItemEvent(const PickupDroppedItemEvent& p_event);
        void HandleDispenseItemToPlayerEvent(const DispenseItemToPlayerEvent& p_event);
        void HandleAddToPlayerInventoryEvent(const SendItemToPlayerInventoryEvent& p_event);
        void HandleRemoveFromPlayerInventoryEvent(const RemoveFromPlayerInventoryEvent& p_event);
        void HandleRemoveFromPlayerEquipmentEvent(const RemoveFromPlayerEquipmentEvent& p_event);

    private:
        void UseItem(ItemBase* p_pItem, int p_iItemIndex);
        void EquipItem(ItemBase* p_pItem, int p_iItemIndex);
        void UnequipItem(ItemBase* p_pItem);
        void DiscardItem(int p_iItemIndex);
        void DiscardEquipment(EquipmentSlot p_enSlot);

        bool m_bShowFullInventoryPrompt = false;
        bool m_bShowTooExpensivePrompt = false;
        bool m_bShowMissingSchematicPrompt = false;
        bool m_bToggleButtonHovered = false;

        const int MAX_SCHEMATICS_PER_RARITY = 99;

        const int MAX_GOLD = 999;
        int m_iGold = 0;

        int m_iToggleButtonIndex;

        int m_iOpenChestIdNum = -1;
        int m_iOpenMerchantIdNum = -1;

        EquipmentItem* m_pEquipment[END_OF_EQUIPMENT];

        int m_iSchematics[END_OF_RARITIES];

        // Shared texture resources for the toggle button
        static std::vector<ImGuiUVSet*> m_vv2ToggleTextureCoords;
        static const std::string m_strToggleTexturePath;
        static inline wolf::Texture* m_pToggleTexture = nullptr;

        static const int TOG_BUTTON_CLOSED;
        static const int TOG_BUTTON_CLOSED_HOVER;
        static const int TOG_BUTTON_OPEN;
        static const int TOG_BUTTON_OPEN_HOVER;
};