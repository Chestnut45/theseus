#pragma once
#include "W_GameObject.h"

// Event that is sent when a boulder is destroyed
struct BoulderDestroyedEvent {
    wolf::GameObject* m_pBoulderObject = nullptr;

    BoulderDestroyedEvent(wolf::GameObject* boulderObject)
        : m_pBoulderObject(boulderObject) {}
};
