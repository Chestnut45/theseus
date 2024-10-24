#pragma once
#include "W_BaseComponent.h"
#include "W_EventManager.h"
#include "ColliderComponent.h"
#include "PlayerController.h"
#include "HealthComponent.h" 
#include "W_Timer.h"

class TrapComponent : public wolf::BaseComponent {
public:
    // Constructor with damage, lifespan, and collider manager
    TrapComponent(float damage, float lifespan, ColliderManager* colliderManager)
        : m_damage(damage), m_lifespan(lifespan), m_colliderManager(colliderManager), m_attackCooldown(1.0f) 
    {
        m_lifespanTimer.Start(); // Start lifespan timer when the trap is created
        m_attackCooldownTimer.Start(); // Start the attack cooldown timer
    }

    // Activate the trap (if not already active)
    void Activate() {
        m_isActive = true;
        m_attackCooldownTimer.Restart();  // Restart cooldown timer
        wolf::Log("Trap has been activated.");
    }

    // Update the trap each frame
    void Update(float delta) {
        if (!m_isActive) return;

        // Check if the trap's lifespan has ended
        if (m_lifespanTimer.Elapsed() >= m_lifespan) {
            wolf::Log("Trap lifespan reached. Deleting trap.");
            GetGameObject()->Delete();
            return;
        }

        // Check if the trap is ready to attack again (based on cooldown)
        if (m_attackCooldownTimer.Elapsed() >= m_attackCooldown) {
            wolf::Log("Trap is ready to attack.");
            CheckForPlayerCollision(delta);
            m_attackCooldownTimer.Restart();  // Reset the cooldown timer after each attack
        }
    }

private:
    float m_damage;                  // Damage dealt by the trap
    float m_lifespan;                // Lifespan of the trap (in seconds)
    float m_attackCooldown;          // Cooldown between attacks (in seconds)
    
    bool m_isActive = false;         // Flag to track whether the trap is active

    wolf::Timer m_lifespanTimer;     // Timer to track the trap's lifespan
    wolf::Timer m_attackCooldownTimer; // Timer to handle attack cooldown

    ColliderManager* m_colliderManager;  // Reference to the collider manager

    // Check for collision with the player and apply damage
    void CheckForPlayerCollision(float delta) {
        auto* trapCollider = GetGameObject()->GetComponent<ColliderComponent>();
        if (!trapCollider || !m_colliderManager) return;

        // Check if the player is standing on the trap
        for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
            auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
            if (playerCollider && m_colliderManager->IsColliding(trapCollider, playerCollider, delta)) {
                auto* playerHealth = playerController.GetGameObject()->GetComponent<HealthComponent>();
                if (playerHealth) {
                    playerHealth->Damage(m_damage);  // Apply damage to the player
                    wolf::Log("Trap dealt " + std::to_string(m_damage) + " damage to player.");
                }
            }
        }
    }
};
