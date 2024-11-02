#include "TriggerComponent.h"
#include "W_GameObject.h"
#include "PlayerController.h"

TriggerComponent::TriggerComponent(ColliderManager* colliderManager, TriggerType type, float trapDamage, float trapLifespan, glm::vec2 trapOffset)
    : m_colliderManager(colliderManager), m_triggerType(type), m_trapDamage(trapDamage), m_trapLifespan(trapLifespan), m_trapOffset(trapOffset) {}

void TriggerComponent::Update(float delta) {
    // Check for player collision and trigger only if not already triggered
    if (!m_triggered && CheckPlayerCollision(delta)) {
        m_triggered = true;
        SpawnTrap();

        // Handle single-use trigger type
        if (m_triggerType == TriggerType::SINGLE_USE) {
            GetGameObject()->Delete();  // Delete after single use
            return;
        }
    }

    // For reusable triggers, reset if trap is inactive (deleted or expired)
    if (m_triggerType == TriggerType::REUSABLE && m_triggered && !IsTrapActive()) {
        m_triggered = false;  // Reset for future use
    }
}

void TriggerComponent::SpawnTrap() {
    // Position the trap relative to the pressure plate's position
    auto* plateTransform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!plateTransform) {
        wolf::Error("TriggerComponent: PressurePlate has no Transform2D component!");
        return;
    }

    glm::vec2 trapPosition = plateTransform->GetGlobalPosition() + m_trapOffset;

    // Create trap object and add components
    auto& trapObj = GetGameObject()->GetScene().CreateObject2D();
    auto& trapSprite = trapObj.AddComponent<wolf::Sprite2D>("data/textures/spiketrap.png");
    trapSprite.SetOriginToCenterOfTexture();

    auto* trapTransform = trapObj.HasAll<wolf::Transform2D>() ? trapObj.GetComponent<wolf::Transform2D>() : &trapObj.AddComponent<wolf::Transform2D>();
    trapTransform->SetPosition(trapPosition);
    trapTransform->SetScale(glm::vec2(3.0f));

    auto& velocity = trapObj.AddComponent<VelocityComponent>();
    velocity.SetVelocity(glm::vec2(0.0f, 0.0f));

    // Add TrapComponent to handle trap logic and behavior, and store a reference to it
    m_trapComponent = &trapObj.AddComponent<TrapComponent>(m_trapDamage, m_trapLifespan, m_colliderManager, this);
    m_trapComponent->Activate();

    // Add collider for the trap
    auto& trapCollider = trapObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::NONE, 0, 1);
    trapCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, 16.0f));
    wolf::Log("Trap spawned and activated.");
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
        if (playerCollider && m_colliderManager->IsColliding(plateCollider, playerCollider, delta)) {
            wolf::Log("Player is colliding with the pressure plate!");
            return true;
        }
    }
    return false;
}
bool TriggerComponent::IsTrapActive() {
    // Check if the trap component's game object still exists
    if (m_trapComponent && m_trapComponent->GetGameObject()) {
        return true;
    } else {
        // The trap has been deleted, so reset the trap component pointer
        m_trapComponent = nullptr;
        return false;
    }
}

void TriggerComponent::SetTriggered(bool triggered) {
    m_triggered = triggered;
}