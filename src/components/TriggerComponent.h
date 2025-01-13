#pragma once

#include <cstdint>
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

// Bitfield of which entity types can activate the trigger
struct EntityListenType
{
    typedef uint32_t type;
    enum : type
    {
        NONE = 0,
        PLAYER = 1,
        MINITAUR = 1 << 1,
        GORGON = 1 << 2,
        HARPY = 1 << 3,
        // NOTE: Continue power of 2 chain for subsequent types to allow bitwise ops
    };
};

class TriggerComponent : public wolf::BaseComponent {
public:

    // Create a trigger component
    TriggerComponent(ColliderManager* colliderManager, TriggerType type, EntityListenType::type entityTypes = EntityListenType::PLAYER);
    ~TriggerComponent();

    // Gets the bitfield of which entity types can activate the trigger
    EntityListenType::type GetEntityListenTypes() const { return m_entityTypes; }

    void Update(float delta);

private:
    bool CheckPlayerCollision(float delta);
    bool CheckEnemyCollision(float delta);
    void OnTrapDestroyed(const TrapDestroyedEvent& event);  

    ColliderManager* m_colliderManager = nullptr;
    bool m_triggered = false;
    TriggerType m_triggerType;
    EntityListenType::type m_entityTypes;
};
