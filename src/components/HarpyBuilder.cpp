#include "HarpyBuilder.h"
#include "HarpyController.h"
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
    auto& collider = harpyObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add ArmourComponent
    auto& armourComponent = harpyObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(data.armour);

    // Add HarpyController and initialize it with data
    auto& controller = harpyObject->AddComponent<HarpyController>();
    controller.Init(data);

    return *harpyObject;
}
