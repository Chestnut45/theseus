#include "DroppedItemComponent.h"

int DroppedItemComponent::m_iNextId = 0;

DroppedItemComponent::~DroppedItemComponent() {
    wolf::EventManager::RemoveListener<DestroyDroppedItemEvent, DroppedItemComponent, &DroppedItemComponent::HandleDestroyDroppedItemEvent>(*this);
}

// Attempt to add this item to the player's inventory
void DroppedItemComponent::PickUpItem() {
    wolf::EventManager::TriggerEvent(PickupDroppedItemEvent(m_iId, m_pItem));
}

// If a dropped item was successfully added to the player's inventory, this handler will be called
void DroppedItemComponent::HandleDestroyDroppedItemEvent(const DestroyDroppedItemEvent& p_event) {
    // Check if we were the item that was picked up
    if (p_event.iDroppedItemId == m_iId) {
        // If we were, delete our gameobject
        this->GetGameObject()->Delete();
    }
}
