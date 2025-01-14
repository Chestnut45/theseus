#pragma once

#include <cstdint>
#include "W_BaseComponent.h"
#include "ColliderComponent.h"
#include "W_EventManager.h"
#include <events/TriggerPurposeFinishedEvent.h>


enum class TriggerType {
    SINGLE_USE,
    REUSABLE,
    CUTSCENE_SINGLE
};

enum class TriggerPurpose {
    NONE,            // No specific action
    SPIKE_TRAP,            // Triggers a trap
    BOULDER_TRAP,
    CUTSCENE         // Triggers a cutscene
};

// Bitfield of which entity types can activate the trigger
struct EntityListenType
{
    typedef uint32_t type;
    enum : type
    {
        NONE = 0,
        PLAYER = 1,
        PLAYER_IGNORE_ROLLING = 1 << 1, // NOTE: Ignores the player ONLY when rolling
        MINITAUR = 1 << 2,
        GORGON = 1 << 3,
        HARPY = 1 << 4,
        // NOTE: Continue power of 2 chain for subsequent types to allow bitwise ops
    };
};

class TriggerComponent : public wolf::BaseComponent {
public:

    // Create a trigger component
    TriggerComponent(ColliderManager* colliderManager, TriggerType type, TriggerPurpose purpose, EntityListenType::type entityTypes = EntityListenType::PLAYER);
    ~TriggerComponent();

    void Update(float delta);

    // Getters for type and purpose
    TriggerType GetTriggerType() const { return m_triggerType; }
    TriggerPurpose GetPurpose() const { return m_purpose; }
    EntityListenType::type GetEntityListenTypes() const { return m_entityTypes; }

private:
    bool CheckPlayerCollision(float delta);
    bool CheckEnemyCollision(float delta);
    void OnPurposeFinished(const TriggerPurposeFinishedEvent& event);
    ColliderManager* m_colliderManager = nullptr;

    // Logic for reusable/single-use triggers
    bool m_triggered = false;

    // Types and purpose of the trigger
    TriggerType m_triggerType;
    TriggerPurpose m_purpose;
    EntityListenType::type m_entityTypes;
};
