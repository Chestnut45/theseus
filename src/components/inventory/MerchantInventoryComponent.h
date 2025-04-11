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
    int iPlayerInventoryIndex;
    bool bWasEquipped;
};

class MerchantInventoryComponent : public InventoryComponent {
    public:
        MerchantInventoryComponent(int p_iSize, int p_iSlotsPerRow, ImVec2 p_v2DrawPos, const std::string& p_strMerchantName, float p_fPercentMarkup, int p_iStartingGold)
            : InventoryComponent(p_iSize, p_iSlotsPerRow, p_v2DrawPos), m_strMerchantName(p_strMerchantName), m_fPercentMarkup(p_fPercentMarkup)
            {
                this->SetGold(p_iStartingGold);

                m_enType = MERCHANT_INVENTORY;
                m_ItemInStasis = ItemInStasis(nullptr, -1);

                wolf::EventManager::AddListener<OpenInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleOpenInventoryEvent>(*this);
                wolf::EventManager::AddListener<CloseInventoryEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleCloseInventoryEvent>(*this);
                wolf::EventManager::AddListener<SellItemToMerchantEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleSellItemToMerchantEvent>(*this);
                wolf::EventManager::AddListener<BoughtItemFromMerchantEvent, MerchantInventoryComponent, &MerchantInventoryComponent::HandleBoughtItemFromMerchantEvent>(*this);
            };

        ~MerchantInventoryComponent();

        // Delete copy constructor/assignment
        MerchantInventoryComponent(const MerchantInventoryComponent&) = delete;
        MerchantInventoryComponent& operator=(const MerchantInventoryComponent&) = delete;

        // Delete move constructor/assignment
        MerchantInventoryComponent(MerchantInventoryComponent&& other) = delete;
        MerchantInventoryComponent& operator=(MerchantInventoryComponent&& other) = delete;

        void SetMarkup(float p_fMarkup) {m_fPercentMarkup = p_fMarkup;};
        float GetMarkup() const {return m_fPercentMarkup;};

        int GetGold() const {return m_iGold;};

        void SetGold(int p_iAmt) {
            m_iGold = p_iAmt;
            if (m_iGold > MAX_GOLD) {
                m_iGold = MAX_GOLD;
            }
        }

        void TakeGold(int p_iAmt) {
            m_iGold -= p_iAmt;
            if (m_iGold < 0) {
                m_iGold = 0;
            }
        }

        void AddGold(int p_iAmt) {
            m_iGold += p_iAmt;
            if (m_iGold > MAX_GOLD) {
                m_iGold = MAX_GOLD;
            }
        }

        void HandleOpenInventoryEvent(const OpenInventoryEvent& p_event);
        void HandleCloseInventoryEvent(const CloseInventoryEvent& p_event);
        void HandleSellItemToMerchantEvent(const SellItemToMerchantEvent& p_event);
        void HandleBoughtItemFromMerchantEvent(const BoughtItemFromMerchantEvent& p_event);

        virtual void ShowInventoryGUI();
        virtual void Close();

    private:
        bool CanAddItem(ItemBase* p_pItem);

        void TakeItemOutOfStasis(bool p_bSold);
        void SellItemToPlayer(int p_iItemIndex, int p_iItemPrice);

        std::string m_strMerchantName;
        float m_fPercentMarkup = 0.0f;

        ItemInStasis m_ItemInStasis;
        bool m_bShowSellForLessPrompt = false;

        const int MAX_GOLD = 999;
        int m_iGold = 0;

};