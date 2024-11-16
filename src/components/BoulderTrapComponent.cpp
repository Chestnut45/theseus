#include "BoulderTrapComponent.h"
#include "PlayerController.h"
#include "EnemyController.h"
#include "W_EventManager.h"
#include "HealthComponent.h"

BoulderTrapComponent::BoulderTrapComponent(ColliderManager* colliderManager, BoulderDirection direction, float speed, float damage)
    : m_colliderManager(colliderManager), m_direction(direction), m_speed(speed), m_damage(damage) {
    wolf::EventManager::AddListener<TriggerEvent, BoulderTrapComponent, &BoulderTrapComponent::OnTriggerEvent>(*this);
}

BoulderTrapComponent::~BoulderTrapComponent() {
    wolf::EventManager::RemoveListener<TriggerEvent, BoulderTrapComponent, &BoulderTrapComponent::OnTriggerEvent>(*this);
}

void BoulderTrapComponent::Update(float delta) {
    if (!m_activated) return;

    MoveBoulder(delta);

    if (CheckCollision(delta)) {
        GetGameObject()->Delete();
        wolf::EventManager::TriggerEvent(TrapDestroyedEvent(GetGameObject()));
    }
}

void BoulderTrapComponent::OnTriggerEvent(const TriggerEvent& event) {
    if (event.m_pTriggerObject == GetGameObject()) {
        m_activated = true;
    }
}

void BoulderTrapComponent::MoveBoulder(float delta) {
    auto* transform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!transform) return;

    glm::vec2 movement(0.0f);

    switch (m_direction) {
        case BoulderDirection::UP:
            movement.y -= m_speed * delta;
            break;
        case BoulderDirection::DOWN:
            movement.y += m_speed * delta;
            break;
        case BoulderDirection::LEFT:
            movement.x -= m_speed * delta;
            break;
        case BoulderDirection::RIGHT:
            movement.x += m_speed * delta;
            break;
    }

    transform->Translate(movement);
}

bool BoulderTrapComponent::CheckCollision(float delta) {
    auto* boulderCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!boulderCollider) return false;

    // Check for collision with players
    if (CheckForPlayerCollision(delta)) return true;

    // Check for collision with enemies
    if (CheckForEnemyCollision(delta)) return true;

    // // Check for collision with walls or environment (basic tilemap check)
    // if (CheckForWallCollision()) {
    //     return true;
    // }

    return false;
}

bool BoulderTrapComponent::CheckForPlayerCollision(float delta) {
    auto* boulderCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!boulderCollider) return false;

    for (auto&& [_, playerController] : GetGameObject()->GetScene().Each<PlayerController>()) {
        auto* playerCollider = playerController.GetGameObject()->GetComponent<ColliderComponent>();
        if (playerCollider && m_colliderManager->IsColliding(*boulderCollider, *playerCollider, delta)) {
            auto* playerHealth = playerController.GetGameObject()->GetComponent<HealthComponent>();
            if (playerHealth) {
                playerHealth->Damage(m_damage);
                return true;
            }
        }
    }
    return false;
}

bool BoulderTrapComponent::CheckForEnemyCollision(float delta) {
    auto* boulderCollider = GetGameObject()->GetComponent<ColliderComponent>();
    if (!boulderCollider) return false;

    for (auto&& [_, enemyController] : GetGameObject()->GetScene().Each<EnemyController>()) {
        auto* enemyCollider = enemyController.GetGameObject()->GetComponent<ColliderComponent>();
        if (enemyCollider && m_colliderManager->IsColliding(*boulderCollider, *enemyCollider, delta)) {
            auto* enemyHealth = enemyController.GetGameObject()->GetComponent<HealthComponent>();
            if (enemyHealth) {
                enemyHealth->Damage(m_damage);
                return true;
            }
        }
    }
    return false;
}
