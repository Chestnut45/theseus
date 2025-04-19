//-----------------------------------------------------------------------------
// File: HarpyBuilder.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Constructs harpy enemies.
//-----------------------------------------------------------------------------
#include "HarpyBuilder.h"
#include "HarpyController.h"
#include "StatusComponent.h"
#include "LightComponent.h"
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
    collider.AddColliderBox(glm::vec2(12.0f, 14.0f), glm::vec2(-6.0f, 6.0f));

    // Add status component
    auto& statusComponent = harpyObject->AddComponent<StatusComponent>();

    // Add HarpyController and initialize it with data
    auto& controller = harpyObject->AddComponent<HarpyController>();
    controller.Init(data);
    controller.SetPlayerID(m_scene.GetPlayerID());

    // Add a light to the harpy
    wolf::GameObject* pLightGO = &m_scene.CreateObject2D();
    auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(0.85f, 0.65f, 0.45f, 0.75f), 95.0f, true);
    harpyObject->AddChild(*pLightGO);
    pLightComponent.Init();
    pLightComponent.SetShadowsEnabled(false);

    return *harpyObject;
}