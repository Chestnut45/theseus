#include "HarpyBuilder.h"
#include "HarpyController.h"
#include "StatusComponent.h"
#include <cassert>

// Build the Harpy GameObject and initialize its components
wolf::GameObject& HarpyBuilder::BuildHarpy(const EnemyData& data, const glm::vec2& position, ColliderManager* pColliderManager) {
    // Create the Harpy GameObject
    wolf::GameObject* harpyObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = harpyObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    harpyObject->AddComponent<HealthComponent>(data.health);
    harpyObject->AddComponent<VelocityComponent>();
    auto& collider = harpyObject->AddComponent<ColliderComponent>(ColliderComponent::HURTBOXDR, false, true);
    collider.AddColliderBox(glm::vec2(24.0f, 26.0f), glm::vec2(-12.0f, 13.0f));

    // Add status component
    auto& statusComponent = harpyObject->AddComponent<StatusComponent>();

    // Add HarpyController and initialize it with data
    auto& controller = harpyObject->AddComponent<HarpyController>();
    controller.Init(data);
    controller.SetPlayerID(m_scene.GetPlayerID());

    return *harpyObject;
}
