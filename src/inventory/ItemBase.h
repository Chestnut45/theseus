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

enum ItemID {
    NONE,
    CONSUMABLE,
    EQUIPMENT,
};

class ItemBase {
    public:
        ItemBase(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, const std::string& p_strImgPath)
            : m_enID(p_enID), m_strName(p_strName), m_strDesc(p_strDesc), m_iValue(p_iValue), m_bStackable(p_bStackable), m_strImgPath(p_strImgPath) {};

        ~ItemBase() {};

        // Once an ID is set you can't change it
        ItemID GetID() const {return m_enID;};

        int GetValue() const {return m_iValue;};
        void SetValue(int p_iValue) {m_iValue = p_iValue;};

        bool IsStackable() const {return m_bStackable;};
        void SetStackable(bool p_bStackable) {m_bStackable = p_bStackable;};

        const std::string& GetName() const {return m_strName;};
        void SetName(const std::string& p_strName) {m_strName = p_strName;};

        const std::string& GetDescription() const {return m_strDesc;};
        void SetDescription(const std::string& p_strDesc) {m_strDesc = p_strDesc;};

        const std::string& GetImagePath() {return m_strImgPath;};
        void SetImgPath(const std::string& p_strImgPath) {m_strImgPath = p_strImgPath;};

    private:
        int m_iValue;
        bool m_bStackable;
        const ItemID m_enID;

        std::string m_strName;
        std::string m_strDesc;
        std::string m_strImgPath;

        // Resources for rendering?
    
    friend class ConsumableItem;
};