#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "TrapComponent.h"
#include "W_EventManager.h"
#include "W_Timer.h"
#include <events/TriggerEvent.h>

class TrapComponent;

enum class TriggerType {
    SINGLE_USE,
    REUSABLE
};

class TriggerComponent : public wolf::BaseComponent {
public:
    TriggerComponent(ColliderManager* colliderManager, TriggerType type, float trapDamage, float trapLifespan, glm::vec2 trapOffset);

    void Update(float delta);
    void SetTriggered(bool triggered);

private:
    void SpawnTrap();
    bool CheckPlayerCollision(float delta);
    bool IsTrapActive();

    ColliderManager* m_colliderManager = nullptr;
    TrapComponent* m_trapComponent = nullptr;

    bool m_triggered = false;
    TriggerType m_triggerType;

    float m_trapDamage;
    float m_trapLifespan;
    glm::vec2 m_trapOffset;  // Offset for trap position
};
