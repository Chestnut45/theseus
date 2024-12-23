#pragma once
#include "W_GameObject.h"

// Event that is sent when a trap is destroyed
struct TrapDestroyedEvent {
    wolf::GameObject* m_pTrapObject = nullptr;

    TrapDestroyedEvent(wolf::GameObject* trapObject)
        : m_pTrapObject(trapObject) {}
};
