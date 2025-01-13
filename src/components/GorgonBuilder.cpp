#include "GorgonBuilder.h"
#include "GorgonController.h"
#include "StatusComponent.h"
#include <cassert>

// Build the Gorgon GameObject and initialize its components
wolf::GameObject& GorgonBuilder::BuildGorgon(const EnemyData& data, const glm::vec2& position, ColliderManager* pColliderManager) {
    // Create the Gorgon GameObject
    wolf::GameObject* gorgonObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = gorgonObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    gorgonObject->AddComponent<HealthComponent>(data.health);
    gorgonObject->AddComponent<VelocityComponent>();
    auto& collider = gorgonObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDR, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 30.0f), glm::vec2(-7.0f, 14.0f));

    // Add status component
    auto& statusComponent = gorgonObject->AddComponent<StatusComponent>();

    // Add GorgonController and initialize it with data
    auto& controller = gorgonObject->AddComponent<GorgonController>();
    controller.Init(data);
    controller.SetPlayerGOId(m_scene.GetPlayerGoId());

    return *gorgonObject;
}
