#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "PlayerController.h"
#include "W_GameObject.h"
#include "W_EventManager.h"
#include <events/TriggerEvent.h>

class TriggerComponent : public wolf::BaseComponent {
public:
    explicit TriggerComponent(ColliderManager* colliderManager)
        : m_colliderManager(colliderManager), m_triggered(false), trapSpawned(false) {}

    void Update(float delta);
    bool IsTriggered() const { return m_triggered; }
    bool IsTrapSpawned() const { return trapSpawned; }
    void SetTrapSpawned(bool value) { trapSpawned = value; }

private:
    bool CheckPlayerCollision(float delta);

    ColliderManager* m_colliderManager;
    bool m_triggered;
    bool trapSpawned;  // This flag tracks whether the trap has already been spawned
};
