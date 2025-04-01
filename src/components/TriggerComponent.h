#pragma once
//-----------------------------------------------------------------------------
// File:			TriggerComponent.h
// Original Author:	Youssef Ashraf
// Modifications:
// ver 1.3
// class responsible for a generalized trigger component
//-----------------------------------------------------------------------------

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
    NONE,
    SPIKE_TRAP,
    BOULDER_TRAP,
    POISON_TRAP,
    LAVA_TRAP,
    CUTSCENE,
    BOSS
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

    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }

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

    // Flag for enabling / disabling triggers
    bool m_active = true;

    // Types and purpose of the trigger
    TriggerType m_triggerType;
    TriggerPurpose m_purpose;
    EntityListenType::type m_entityTypes;
};
