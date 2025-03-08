//-----------------------------------------------------------------------------
// File: PlaceableItem.h
// Original Author: Nguyễn Minh Nhật
// 
//-----------------------------------------------------------------------------
# pragma once

#include "ItemBase.h"
#include <format>

class PlaceableItem : public ItemBase 
{
public:
    PlaceableItem(ItemID p_enID, const std::string& p_strName, const std::string& p_strDesc, int p_iValue, bool p_bStackable, int p_iTextureFrameIndex, Rarity p_enRarity, int p_iNumUses)
        : ItemBase(p_enID, p_strName, p_strDesc, p_iValue, p_bStackable, p_iTextureFrameIndex, p_enRarity){};
    
    ~PlaceableItem() {};

    // Delete copy constructor/assignment
    PlaceableItem(const PlaceableItem&) = delete;
    PlaceableItem& operator=(const PlaceableItem&) = delete;

    // Delete move constructor/assignment
    PlaceableItem(PlaceableItem&& other) = delete;
    PlaceableItem& operator=(PlaceableItem&& other) = delete;

private:
};

struct BeginPlacingPlaceableEvent {
    ItemBase* pItem;
};

struct EndPlacingPlaceableEvent {
    ItemBase* pItem;
};

struct RetrievePlaceableEvent {
    ItemBase* pItem;
};