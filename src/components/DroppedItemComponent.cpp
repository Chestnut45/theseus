#include "DroppedItemComponent.h"

//-----------------------------------------------------------------------------
// File:            DroppedItemComponent.cpp
// Original Author: Aurora Ryder
//
// A class representing an item that has been dropped by some entity
// (Note that this class is essentially a component wrapper for ItemBases)
//-----------------------------------------------------------------------------

// Static ID number for resource management
int DroppedItemComponent::m_iNextId = 0;

DroppedItemComponent::~DroppedItemComponent() {
    wolf::EventManager::RemoveListener<DestroyDroppedItemEvent, DroppedItemComponent, &DroppedItemComponent::HandleDestroyDroppedItemEvent>(*this);
}

// Updates the component and deletes it if the DroppedItem reaches the end
// of its lifespan without being picked up by the player
// > p_fDelta: the delta time
void DroppedItemComponent::Update(float p_fDelta) {
    // If this drop item can despawn
    if (m_fLifeSpan > 0) {
        // And it has reached the end of its lifespan
        if (m_fTimeSpentAlive > m_fLifeSpan) {
            // Kill it
            this->GetGameObject()->Delete();
        }

        // Otherwise, just update the timer
        m_fTimeSpentAlive += p_fDelta;
    }
}

// Attempts to add this DroppedItem to the player's inventory by sending a PickupDroppedItemEvent.
// If the player does not have room in their inventory, the item stays on the ground
void DroppedItemComponent::PickUpItem() {
    wolf::EventManager::TriggerEvent(PickupDroppedItemEvent(m_iId, m_pItem));
}

// If a dropped item was successfully added to the player's inventory, this handler will be called
// and the DroppedItem will be deleted as its in-world representation is no longer needed
void DroppedItemComponent::HandleDestroyDroppedItemEvent(const DestroyDroppedItemEvent& p_event) {
    // Check if we were the item that was picked up
    if (p_event.iDroppedItemId == m_iId) {
        // If we were, delete our gameobject
        this->GetGameObject()->Delete();
    }
}
