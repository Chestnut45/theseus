#include "TrapComponent.h"
#include "PlayerController.h"
#include "HealthComponent.h"

TrapComponent::TrapComponent(float damage, float lifespan, ColliderManager* colliderManager)
    : m_damage(damage), m_lifespan(lifespan), m_colliderManager(colliderManager), m_attackCooldown(1.0f) {
    m_lifespanTimer.Start();
    m_attackCooldownTimer.Start();
}

void TrapComponent::Activate() {
    m_isActive = true;
    m_attackCooldownTimer.Restart();
}

void TrapComponent::Update(float delta) {
    if (!m_isActive) return;

    if (m_lifespanTimer.Elapsed() >= m_lifespan) {
        GetGameObject()->Delete();
        return;
    }

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
