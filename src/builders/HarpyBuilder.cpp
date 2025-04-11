//-----------------------------------------------------------------------------
// File: HarpyBuilder.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Constructs harpy enemies.
//-----------------------------------------------------------------------------
#include "HarpyBuilder.h"
#include "HarpyController.h"
#include "StatusComponent.h"
#include <cassert>

// Build the Harpy GameObject and initialize its components
wolf::GameObject& HarpyBuilder::BuildHarpy(const EnemyData& data, const glm::vec2& position)
{
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
    collider.AddColliderBox(glm::vec2(18.0f, 20.0f), glm::vec2(-9.0f, 10.0f));

    // Add status component
    auto& statusComponent = harpyObject->AddComponent<StatusComponent>();

    // Add HarpyController and initialize it with data
    auto& controller = harpyObject->AddComponent<HarpyController>();
    controller.Init(data);
    controller.SetPlayerID(m_scene.GetPlayerID());

    return *harpyObject;
}