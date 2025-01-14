#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_Timer.h"
#include <events/TrapDestroyedEvent.h>
#include "TriggerComponent.h"

class TrapComponent : public wolf::BaseComponent {
public:
    TrapComponent(float damage, float lifespan, ColliderManager* colliderManager, float initialDelay = 0.0f, EntityListenType::type entityTypes = EntityListenType::PLAYER);
    void Update(float delta);

private:
    bool CheckForPlayerCollision(float delta);
    bool CheckForEnemyCollision(float delta);

    float m_damage;
    float m_lifespan;
    float m_initialDelay;
    bool m_triggered = false;

    bool m_isActive = true; // Trap is active upon creation
    EntityListenType::type m_entityTypes;
    wolf::Timer m_lifespanTimer;
    ColliderManager* m_pColliderManager = nullptr;
};
