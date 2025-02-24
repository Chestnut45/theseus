//-----------------------------------------------------------------------------
// File:			TrapComponent.h
// Original Author:	Youssef Ashraf
// ver 1.1
// class responsible for spike traps
//-----------------------------------------------------------------------------
#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_Timer.h"
#include <events/TriggerPurposeFinishedEvent.h>
#include "TriggerComponent.h"

class TrapComponent : public wolf::BaseComponent {
public:
    TrapComponent(TriggerComponent* pCreatorTrigger, float damage, float lifespan, ColliderManager* pColliderManager, float initialDelay = 0.0f);
    void Update(float delta);

private:
    bool CheckForPlayerCollision(float delta);
    bool CheckForEnemyCollision(float delta);

    float m_damage;
    float m_lifespan;
    float m_initialDelay;
    bool m_triggered = false;

    bool m_isActive = true; // Trap is active upon creation
    wolf::Timer m_lifespanTimer;
    ColliderManager* m_pColliderManager = nullptr;
    TriggerComponent* m_pCreatorTrigger = nullptr;
    EntityListenType::type m_entityTypes;
};
