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

struct GoldEvent {
    int iAmt;
};

struct RemoveFromPlayerInventoryEvent {
    std::string strItemName;

    // Providing the index is optional, but it should be used whenever possible
    // as it ensures that we are deleting a specific instance of an item, rather
    // rather than the first one that we find.
    int iIndex = -1;

    // This is an optional variable that lets us know if the item is being removed because it was sold
    bool bWasSold = false;
};