#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_Timer.h"
#include <events/TrapDestroyedEvent.h>

class TrapComponent : public wolf::BaseComponent {
public:
    TrapComponent(float damage, float lifespan, ColliderManager* colliderManager);
    void Update(float delta);

private:
    bool CheckForPlayerCollision(float delta);

    float m_damage;
    float m_lifespan;
    float m_attackCooldown;

    bool m_isActive = true; // Trap is active upon creation
    wolf::Timer m_lifespanTimer;
    wolf::Timer m_attackCooldownTimer;
    ColliderManager* m_colliderManager = nullptr;
};
