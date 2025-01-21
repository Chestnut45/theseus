#pragma once

#include <string>

struct PercentHealthItemEvent {
    float fHealthChangeAmt;
};

struct PercentStaminaItemEvent {
    float fStaminaAmt;
};

struct FlatHealthItemEvent {
    float fHealthChangeAmt;
};

struct FlatStaminaItemEvent {
    float fStaminaAmt;
};

struct ApplyStatusEffectEvent {
    // Note that StatusEffectComponent and StatusEffectItems both use an enum to represent the type, but I did not think
    // it was a good idea to include the whole component header here so the event itself uses an integer.
    int iType;
    float fDuration;
};

struct RemoveFromPlayerInventoryEvent {
    std::string strItemName;

    // Providing the index is optional, but it should be used whenever possible
    // as it ensures that we are deleting a specific instance of an item, rather
    // rather than the first one that we find.
    int iIndex = -1;

    // If the item was sold to someone, we'll want to know how much it was sold for
    int iItemSoldFor = 0;
};

struct RemoveFromPlayerEquipmentEvent {
    RemoveFromPlayerEquipmentEvent(int p_iSlot, int p_iSalePrice) : iEquipSlot(p_iSlot), iItemSoldFor(p_iSalePrice) {};
    RemoveFromPlayerEquipmentEvent(int p_iSlot) : iEquipSlot(p_iSlot) {};

    int iEquipSlot;

    // If the equipment was sold to someone, we'll want to know how much it was sold for
    int iItemSoldFor = 0;
};