#include "GorgonBuilder.h"
#include "GorgonController.h"
#include <cassert>

// Build the Gorgon GameObject and initialize its components
wolf::GameObject& GorgonBuilder::BuildGorgon(const EnemyData& data, const glm::vec2& position, ColliderManager* pColliderManager) {
    // Create the Gorgon GameObject
    wolf::GameObject* GorgonObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = GorgonObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    GorgonObject->AddComponent<HealthComponent>(data.health);
    GorgonObject->AddComponent<VelocityComponent>();
    auto& collider = GorgonObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add ArmourComponent
    auto& armourComponent = GorgonObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(data.armour);

    // Add GorgonController and initialize it with data
    auto& controller = GorgonObject->AddComponent<GorgonController>();
    controller.Init(data);

    return *GorgonObject;
}
