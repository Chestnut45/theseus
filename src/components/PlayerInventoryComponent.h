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
                // We need to fill the equipment array with invalid indices so we don't
                // automatically equip the first item added to the inventory
                for (int i = 0; i < END_OF_EQUIPMENT; i++) {
                    m_iEquipmentSlots[i] = -1;
                }
            };
            
        ItemBase* GetEquippedItem(EquipmentSlot p_enSlot);

        virtual void ShowInventoryGUI();

        void AddGold(int p_iAmt);
        bool TakeGold(int p_iAmt);

        inline int GetGold() {return m_iGold;};

    private:
        void UseItem(ItemBase* p_pItem, int p_iItemIndex);
        void EquipItem(ItemBase* p_pItem, int p_iItemIndex);
        void UnequipItem(ItemBase* p_pItem);
        void DiscardItem(int p_iItemIndex);

        const int MAX_GOLD = 999;
        int m_iGold = 0;

        int m_iEquipmentSlots[END_OF_EQUIPMENT - 1];
};