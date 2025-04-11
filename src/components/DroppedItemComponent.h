#pragma once

//-----------------------------------------------------------------------------
// File:            DroppedItemComponent.h
// Original Author: Aurora Ryder
//
// A class representing an item that has been dropped by some entity
// (Note that this class is essentially a component wrapper for ItemBases)
//-----------------------------------------------------------------------------

#include <W_BaseComponent.h>
#include <W_EventManager.h>
#include <W_GameObject.h>

#include "ItemBase.h"
#include <DroppedItemEvents.h>

class DroppedItemComponent : public wolf::BaseComponent {
    public:
        DroppedItemComponent(ItemBase* p_pItem, float p_fLifespan) : m_pItem(p_pItem), m_iId(m_iNextId), m_fLifeSpan(p_fLifespan)
            {
                wolf::EventManager::AddListener<DestroyDroppedItemEvent, DroppedItemComponent, &DroppedItemComponent::HandleDestroyDroppedItemEvent>(*this);
                m_iNextId++;
            };

        ~DroppedItemComponent();

        // Delete copy constructor/assignment
        DroppedItemComponent(const DroppedItemComponent&) = delete;
        DroppedItemComponent& operator=(const DroppedItemComponent&) = delete;

        // Delete move constructor/assignment
        DroppedItemComponent(DroppedItemComponent&& other) = delete;
        DroppedItemComponent& operator=(DroppedItemComponent&& other) = delete;

        void Update(float p_fDelta);

        void PickUpItem();

        int GetId() const {return m_iId;};

        void HandleDestroyDroppedItemEvent(const DestroyDroppedItemEvent& p_event);

    private:
        static int m_iNextId;
        const int m_iId;

        float m_fLifeSpan = -1.0f; // Note that a lifespan of -1 means that the item will NOT despawn
        float m_fTimeSpentAlive = 0.0f;

        ItemBase* m_pItem; // The item this component is representing
};