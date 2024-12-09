#pragma once
#include "W_GameObject.h" 

// Forward declare enums to avoid circular dependency
enum class TriggerType;
enum class TriggerPurpose;

struct TriggerEvent {
    wolf::GameObject* m_pTriggerObject = nullptr;
    TriggerType m_triggerType;
    TriggerPurpose m_purpose;

    TriggerEvent(wolf::GameObject* triggerObject, TriggerType type, TriggerPurpose purpose)
        : m_pTriggerObject(triggerObject), m_triggerType(type), m_purpose(purpose) {}
};