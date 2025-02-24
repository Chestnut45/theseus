#pragma once
#include "inventory/ItemBase.h"

// Created by Aurora Ryder for use with DroppedItems

struct PickupDroppedItemEvent {
    int iDroppedItemId;
    ItemBase* pItem;
};

struct DestroyDroppedItemEvent {
    int iDroppedItemId;
};