#pragma once

//-----------------------------------------------------------------------------
// File:            MerchantInventoryComponent.h
// Original Author: Aurora Ryder
//
// A class representing a given merchant's inventory
//-----------------------------------------------------------------------------

#include "InventoryComponent.h"

struct ItemInStasis {
    ItemBase* pItem;
    int iInventoryIndex;
};

class MerchantInventoryComponent : public InventoryComponent {
    public:
        MerchantInventoryComponent(int p_iSize, int p_iSlotsPerRow, const std::string& p_strTexture, const glm::vec2& p_v2TexFrameSize)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_strTexture, p_v2TexFrameSize)
            {
                m_enType = MERCHANT_INVENTORY;
                m_ItemInStasis = ItemInStasis(nullptr, -1);

                wolf::EventManager::AddListener<OpenInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleOpenInventoryEvent>(*this);

            };

        ~MerchantInventoryComponent();

        // Delete copy constructor/assignment
        MerchantInventoryComponent(const MerchantInventoryComponent&) = delete;
        MerchantInventoryComponent& operator=(const MerchantInventoryComponent&) = delete;

        // Delete move constructor/assignment
        MerchantInventoryComponent(MerchantInventoryComponent&& other) = delete;
        MerchantInventoryComponent& operator=(MerchantInventoryComponent&& other) = delete;

        bool StockMerchantFromFile(const std::string& p_strFilePath);

        virtual void Open();
        virtual void Close();
        virtual void ToggleOpen();

        bool CanAddItem(ItemBase* p_pItem);

        void SetMarkup(float p_fMarkup) {m_fMarkupValue = p_fMarkup;};
        float GetMarkup() const {return m_fMarkupValue;};

        int GetGold() const {return m_iGold;};

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleSellItemToMerchantEvent(const SellItemToMerchantEvent& p_event);
        void HandleBuyItemFromMerchantEvent(const BuyItemFromMerchantEvent& p_event);

        virtual void ShowInventoryGUI();

    private:
        std::string m_strMerchantName;
        float m_fMarkupValue = 0.0f;

        ItemInStasis m_ItemInStasis;
        bool m_bShowSellForLessPrompt;

        const int MAX_GOLD = 999;
        int m_iGold = 0;

};