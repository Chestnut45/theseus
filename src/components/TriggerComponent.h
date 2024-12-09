#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_EventManager.h"
#include <events/TriggerEvent.h>
#include <events/TrapDestroyedEvent.h>

enum class TriggerType {
    SINGLE_USE,
    REUSABLE,
};

enum class TriggerPurpose {
    NONE,            // No specific action
    TRAP,            // Triggers a trap
    CUTSCENE         // Triggers a cutscene
};


class TriggerComponent : public wolf::BaseComponent {
public:
    TriggerComponent(ColliderManager* colliderManager, TriggerType type, TriggerPurpose purpose);
    ~TriggerComponent();

    void Update(float delta);
    // Getters for type and purpose
    TriggerType GetTriggerType() const { return m_triggerType; }
    TriggerPurpose GetPurpose() const { return m_purpose; }

private:
    bool CheckPlayerCollision(float delta);
    void OnTrapDestroyed(const TrapDestroyedEvent& event);
    ColliderManager* m_colliderManager = nullptr;

    // Logic for reusable/single-use triggers
    bool m_triggered = false;

    // Types and purpose of the trigger
    TriggerType m_triggerType;
    TriggerPurpose m_purpose;
};
