#pragma once
#include "W_BaseComponent.h"
#include "W_EventManager.h"
#include "ColliderComponent.h"
#include "PlayerController.h"
#include "W_GameObject.h"
#include "TriggerEvent.h" // Event triggered when pressure plate is activated

class TriggerComponent : public wolf::BaseComponent {
public:
    TriggerComponent(ColliderManager* colliderManager, const std::string& triggerName)
        : m_colliderManager(colliderManager), m_triggered(false), m_triggerName(triggerName) {}

    void Update(float delta) 
    {
        if (!m_colliderManager) {
            wolf::Error("TriggerComponent: ColliderManager is null!");
            return;
        }
        
        // Use GetGameObject() instead of directly accessing m_pGameObject
        if (!GetGameObject()) {
            wolf::Error("TriggerComponent: GameObject is null in Update!");
            return;
        }

        if (!m_triggered && CheckPlayerCollision(delta)) {
            wolf::EventManager::TriggerEvent(TriggerEvent("PressurePlateSteppedOn", GetGameObject()));
            m_triggered = true;
        }
    }

private:
    ColliderManager* m_colliderManager;
    bool m_triggered;
    std::string m_triggerName; // Store the name of the trigger

    bool CheckPlayerCollision(float delta) {
        wolf::GameObject* plateGameObject = GetGameObject();
        if (!plateGameObject) {
            // If the GameObject is null, log a warning and return false to prevent further issues
            wolf::Error("TriggerComponent: GameObject is null!");
            return false;
        }

        // Get the collider component of the pressure plate
        auto* plateCollider = plateGameObject->GetComponent<ColliderComponent>();
        if (!plateCollider) {
            wolf::Error("TriggerComponent: Plate collider is null!");
            return false;
        }

        // Find and set the player as the target
        bool playerFound = false;
        for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
            wolf::GameObject* playerGameObject = playerController.GetGameObject();
            // wolf::Log("Player found with GameObject ID: " + std::to_string(playerGameObject->GetID()));
            playerFound = true; // Mark that we found a player
            auto* playerCollider = playerGameObject->GetComponent<ColliderComponent>();

            // Pass delta to the collision check
            if (playerCollider && m_colliderManager->IsColliding(plateCollider, playerCollider, delta)) {
                // wolf::Log("Player is colliding with the pressure plate!");
                return true;  // Return true immediately if a collision is detected
            }
        }
        return false; // Return false if no collision was found
    }
};
