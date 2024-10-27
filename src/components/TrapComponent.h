#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_Timer.h"

class TrapComponent : public wolf::BaseComponent {
public:
    TrapComponent(float damage, float lifespan, ColliderManager* colliderManager);
    void Activate();
    void Update(float delta);

private:
    bool CheckForPlayerCollision(float delta);

    float m_damage;
    float m_lifespan;
    float m_attackCooldown;
    
    bool m_isActive = false;
    wolf::Timer m_lifespanTimer;
    wolf::Timer m_attackCooldownTimer;
    ColliderManager* m_colliderManager;
};