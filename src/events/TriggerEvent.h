#pragma once
#include "W_GameObject.h"  // Include for wolf::GameObject*

// Forward declare enums to avoid circular dependency
enum class TriggerType;
enum class TrapType;

struct TriggerEvent {
    wolf::GameObject* m_pTriggerObject = nullptr;
    TriggerType m_triggerType;
    TrapType m_trapType;

    TriggerEvent(wolf::GameObject* triggerObject, TriggerType type, TrapType trapType)
        : m_pTriggerObject(triggerObject), m_triggerType(type), m_trapType(trapType) {}
};
