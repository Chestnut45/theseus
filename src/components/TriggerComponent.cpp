#include "TriggerComponent.h"

void TriggerComponent::Update(float delta) {
    if (!m_triggered && !trapSpawned && CheckPlayerCollision(delta)) {
        m_triggered = true;  // Only trigger if not already triggered
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

    // Log when searching for player collision

    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerGameObject = playerController.GetGameObject();
        auto* playerCollider = playerGameObject->GetComponent<ColliderComponent>();

        if (playerCollider && m_colliderManager->IsColliding(plateCollider, playerCollider, delta)) {
            wolf::Log("Player is colliding with the pressure plate!");  // Log collision
            return true;
        }
    }

    return false;
}
