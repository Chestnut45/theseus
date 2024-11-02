#include "TrapComponent.h"
#include "PlayerController.h"
#include "HealthComponent.h"

TrapComponent::TrapComponent(float damage, float lifespan, ColliderManager* colliderManager)
    : m_damage(damage), m_lifespan(lifespan), m_colliderManager(colliderManager), m_attackCooldown(1.0f) {
    // Start timers immediately, making the trap active upon creation
    m_lifespanTimer.Start();
    m_attackCooldownTimer.Start();
}

void TrapComponent::Update(float delta) {
    if (!m_isActive) return;

    // Check if the lifespan has expired
    if (m_lifespanTimer.Elapsed() >= m_lifespan) {
        // Send event before deleting the trap
        wolf::Log("Triggering TrapDestroyedEvent for GameObject " + std::to_string(GetGameObject()->GetID()));
        wolf::EventManager::TriggerEvent(TrapDestroyedEvent(GetGameObject()));
        GetGameObject()->Delete();
        return;
    }

    // Handle player collision and attack cooldown
    if (m_attackCooldownTimer.Elapsed() >= m_attackCooldown) {
        if (CheckForPlayerCollision(delta)) {
            m_attackCooldownTimer.Restart();
        }
    }
}

bool TrapComponent::CheckForPlayerCollision(float delta) {
    auto* trapCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!trapCollider) return false;

    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(trapCollider, playerCollider, delta)) {
            auto* playerHealth = playerController.GetGameObject()->GetComponent<HealthComponent>();
            if (playerHealth) {
                playerHealth->Damage(m_damage);
                return true;
            }
        }
    }
    return false;
}
