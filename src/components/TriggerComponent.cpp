#include "TriggerComponent.h"
#include "W_GameObject.h"
#include "PlayerController.h"
#include "MinitaurController.h"
#include "GorgonController.h"
#include "HarpyController.h"
#include "W_EventManager.h"


TriggerComponent::TriggerComponent(ColliderManager* colliderManager, TriggerType type, EntityListenType::type entityTypes)
    : m_colliderManager(colliderManager), m_triggerType(type), m_entityTypes(entityTypes), m_triggered(false) {
    // Register to listen for TrapDestroyedEvent
    wolf::EventManager::AddListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
}

TriggerComponent::~TriggerComponent() {
    // Unregister from the event to avoid dangling references
    wolf::EventManager::RemoveListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
}


void TriggerComponent::Update(float delta) {
    if (!m_triggered && (CheckPlayerCollision(delta) || CheckEnemyCollision(delta))) {
        m_triggered = true;

        // Dispatch the TriggerEvent
        wolf::EventManager::TriggerEvent(TriggerEvent(GetGameObject(), m_triggerType));

        // Delete the object if it's a single-use trigger
        if (m_triggerType == TriggerType::SINGLE_USE || m_triggerType == TriggerType::CUTSCENE_SINGLE) {
            GetGameObject()->Delete();
        }
    }
}

bool TriggerComponent::CheckPlayerCollision(float delta)
{
    // Early out if not listening for player
    if (!(m_entityTypes & EntityListenType::PLAYER || m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING)) return false;

    auto* plateGameObject = GetGameObject();
    if (!plateGameObject) {
        wolf::Error("TriggerComponent: GameObject is null!");
        return false;
    }

    auto* plateCollider = plateGameObject->GetComponent<ColliderComponent>();
    if (!plateCollider) {
        wolf::Error("TriggerComponent: Plate collider is null!");
        return false;
    }

    // Check for collision with any player in the scene
    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {

        // Ignore player if rolling
        if (m_entityTypes & EntityListenType::PLAYER_IGNORE_ROLLING &&
            playerController.GetPlayerAction() == PlayerController::PlayerAction::ROLLING)
        {
            continue;
        }
        
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(*plateCollider, *playerCollider, delta)) {
            // wolf::Log("Player is colliding with the pressure plate!");
            return true;
        }
    }
    return false;
}

bool TriggerComponent::CheckEnemyCollision(float delta)
{
    auto* plateGameObject = GetGameObject();
    if (!plateGameObject) {
        wolf::Error("TriggerComponent: GameObject is null!");
        return false;
    }

    auto* plateCollider = plateGameObject->GetComponent<ColliderComponent>();
    if (!plateCollider) {
        wolf::Error("TriggerComponent: Plate collider is null!");
        return false;
    }

    // Minitaurs
    if (m_entityTypes & EntityListenType::MINITAUR)
    {
        for (auto&& [_, minitaur] : GetGameObject()->GetScene().Each<MinitaurController>())
        {
            auto* pCollider = minitaur.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    // Gorgons
    if (m_entityTypes & EntityListenType::GORGON)
    {
        for (auto&& [_, gorgon] : GetGameObject()->GetScene().Each<GorgonController>())
        {
            auto* pCollider = gorgon.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    // Harpies
    if (m_entityTypes & EntityListenType::HARPY)
    {
        for (auto&& [_, harpy] : GetGameObject()->GetScene().Each<HarpyController>())
        {
            auto* pCollider = harpy.GetGameObject()->GetComponent<ColliderComponent>();
            if (pCollider && pCollider->IsActive() && m_colliderManager->IsColliding(*plateCollider, *pCollider, delta))
            {
                return true;
            }
        }
    }

    return false;
}

void TriggerComponent::OnTrapDestroyed(const TrapDestroyedEvent& event) {
    if (m_triggerType == TriggerType::REUSABLE && m_triggered) {
        m_triggered = false;  // Allow the trigger to be reused once the trap is destroyed
    }
}