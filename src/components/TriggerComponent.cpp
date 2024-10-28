#include "TriggerComponent.h"

void TriggerComponent::Update(float delta) {
    // Check if the player has collided with the trigger and it has not yet been triggered
    if (!m_triggered && !trapSpawned && CheckPlayerCollision(delta)) {
        m_triggered = true;  // Set triggered flag
        trapSpawned = true;  // Prevent future triggering

        // Trigger the event for the pressure plate
        wolf::EventManager::TriggerEvent(TriggerEvent("PressurePlateSteppedOn", GetGameObject()));
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
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerGameObject = playerController.GetGameObject();
        auto* playerCollider = playerGameObject->GetComponent<ColliderComponent>();

        if (playerCollider && m_colliderManager->IsColliding(plateCollider, playerCollider, delta)) {
            wolf::Log("Player is colliding with the pressure plate!");
            return true;
        }
    }

    return false;
}