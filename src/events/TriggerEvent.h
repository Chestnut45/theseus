#pragma once
#include "W_GameObject.h"  // Include for wolf::GameObject*

// Define TriggerType here to avoid multiple definitions
enum class TriggerType;
struct TriggerEvent {
    wolf::GameObject* m_pTriggerObject = nullptr;
    TriggerType m_triggerType;

    TriggerEvent(wolf::GameObject* triggerObject, TriggerType type)
        : m_pTriggerObject(triggerObject), m_triggerType(type) {}
};
