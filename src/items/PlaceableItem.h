//-----------------------------------------------------------------------------
// File: PlaceableItem.h
// Original Author: Nguyễn Minh Nhật
// 
//-----------------------------------------------------------------------------
# pragma once

#include "ItemBase.h"
#include <format>

enum PlaceableType {
    PORTAL,
    END_OF_PLACEABLE_TYPES, // Sentinel value for iteration
};

class PlaceableItem : public ItemBase 
{
public:
    PlaceableItem(ItemID p_enID, PlaceableType p_plcType, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, Rarity p_enRarity)
        : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex, p_enRarity)
        {
            m_type = p_plcType;
        }
    
    ~PlaceableItem() {};

    // Delete copy constructor/assignment
    PlaceableItem(const PlaceableItem&) = delete;
    PlaceableItem& operator=(const PlaceableItem&) = delete;

    // Delete move constructor/assignment
    PlaceableItem(PlaceableItem&& other) = delete;
    PlaceableItem& operator=(PlaceableItem&& other) = delete;

    PlaceableType GetType() const {
        return m_type;
    };

private:
    PlaceableType m_type = END_OF_PLACEABLE_TYPES;
};

struct BeginPlacingPlaceableEvent {
    PlaceableItem* pItem;
};

struct EndPlacingPlaceableEvent {
    PlaceableItem* pItem = nullptr;
};

struct RetrievePlaceableEvent {
    PlaceableType pcTpye;
    glm::ivec2 tilePos;
};

struct DestroyPlaceableEvent {
    PlaceableType pcTpye;
    glm::ivec2 tilePos;
};
