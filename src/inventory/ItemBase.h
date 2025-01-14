#pragma once

//-----------------------------------------------------------------------------
// File:            ItemBase.h
// Original Author: Aurora Ryder
//
// A class representing the core functionality of all items
//
// Anything that can be added to the inventory will inherit from this
//-----------------------------------------------------------------------------

#include <string>
#include "InventoryEvents.h"

enum ItemID {
    NONE = 1,
    CONSUMABLE,
    EQUIPMENT,
    GOLD,
    SCHEMATIC,
};

// Each of these should have a corresponding entry in the RarityColors array
enum Rarity {
    COMMON,
    UNCOMMON,
    RARE,
    EPIC,
    LEGENDARY,
    END_OF_RARITIES,
};

// For use with ImGui
struct RGBIntColor {
    int r;
    int g;
    int b;
};

// For use with ImGui
const RGBIntColor RarityColors[] {
    {255, 255, 255},   // COMMON
    {30, 255, 0},      // UNCOMMON
    {0, 112, 221},     // RARE
    {163, 53, 238},    // EPIC
    {255, 128, 0},     // LEGENDARY
};

class ItemBase {
    public:
        ItemBase(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, Rarity p_enRarity)
            : m_enID(p_enID), m_strName(p_strName), m_strDesc(p_strDesc), m_iValue(p_iValue), m_bStackable(p_bStackable), m_iTextureFrameIndex(p_iTextureFrameIndex), m_enRarity(p_enRarity)
            {};

        virtual ~ItemBase() {};

        // Delete copy constructor/assignment
        ItemBase(const ItemBase&) = delete;
        ItemBase& operator=(const ItemBase&) = delete;

        // Delete move constructor/assignment
        ItemBase(ItemBase&& other) = delete;
        ItemBase& operator=(ItemBase&& other) = delete;

        // Once an ID is set you can't change it
        ItemID GetID() const {return m_enID;};
        Rarity GetRarity() const {return m_enRarity;};

        int GetValue() const {return m_iValue;};
        void SetValue(int p_iValue) {m_iValue = p_iValue;};

        // Once you've set an item's stackability you can't change it
        bool IsStackable() const {return m_bStackable;};

        const std::string& GetName() const {return m_strName;};
        void SetName(const std::string& p_strName) {m_strName = p_strName;};

        const std::string& GetDescription() const {return m_strDesc;};
        void SetDescription(const std::string& p_strDesc) {m_strDesc = p_strDesc;};

        int GetTextureFrameIndex() const {return m_iTextureFrameIndex;};
        void SetTextureFrameIndex(int p_iIndex) {m_iTextureFrameIndex = p_iIndex;};

        inline virtual std::string GetToolTipText() const {
            return m_strDesc;
        };

    protected:
        int m_iValue;
        int m_iTextureFrameIndex;
        bool m_bStackable;

        const ItemID m_enID;
        const Rarity m_enRarity;

        std::string m_strName;
        std::string m_strDesc;
};