#include "TriggerComponent.h"
#include "W_GameObject.h"
#include "PlayerController.h"
#include "W_EventManager.h"


TriggerComponent::TriggerComponent(ColliderManager* colliderManager, TriggerType type)
    : m_colliderManager(colliderManager), m_triggerType(type), m_triggered(false) {
    // Register to listen for TrapDestroyedEvent
    wolf::EventManager::AddListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
}

TriggerComponent::~TriggerComponent() {
    // Unregister from the event to avoid dangling references
    wolf::EventManager::RemoveListener<TrapDestroyedEvent, TriggerComponent, &TriggerComponent::OnTrapDestroyed>(*this);
}


void TriggerComponent::Update(float delta) {
    if (!m_triggered && CheckPlayerCollision(delta)) {
        m_triggered = true;

        // Fire a general trigger event
        wolf::EventManager::TriggerEvent(TriggerEvent(GetGameObject(), m_triggerType));

        // If single-use, delete the trigger
        if (m_triggerType == TriggerType::SINGLE_USE) {
            GetGameObject()->Delete();
        }
    }
}

bool TriggerComponent::CheckPlayerCollision(float delta) {
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
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(*plateCollider, *playerCollider, delta)) {
            // wolf::Log("Player is colliding with the pressure plate!");
            return true;
        }
    }
    return false;
}

void TriggerComponent::OnTrapDestroyed(const TrapDestroyedEvent& event) {
    if (m_triggerType == TriggerType::REUSABLE && m_triggered) {
        m_triggered = false;  // Allow the trigger to be reused once the trap is destroyed
    }
}