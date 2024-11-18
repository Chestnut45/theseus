#pragma once
#include "inventory/ItemBase.h"

struct PickupDroppedItemEvent {
    int iDroppedItemId;
    ItemBase* pItem;
};

struct DestroyDroppedItemEvent {
    int iDroppedItemId;
};