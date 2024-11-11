#pragma once
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_EventManager.h"
#include <events/TriggerEvent.h>
#include <events/TrapDestroyedEvent.h>


enum class TriggerType {
    SINGLE_USE,
    REUSABLE,
    CUTSCENE_SINGLE
};

class TriggerComponent : public wolf::BaseComponent {
public:
    TriggerComponent(ColliderManager* colliderManager, TriggerType type);
    ~TriggerComponent();


    void Update(float delta);

private:
    bool CheckPlayerCollision(float delta);
    void OnTrapDestroyed(const TrapDestroyedEvent& event);  

    ColliderManager* m_colliderManager = nullptr;
    bool m_triggered = false;
    TriggerType m_triggerType;
};
