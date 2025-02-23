//-----------------------------------------------------------------------------
// File: MinitaurBuilder.cpp
// Original Author:	???
// Modifications: Nguyễn Minh Nhật
// Constructs minitaur enemies.
//-----------------------------------------------------------------------------

#include "MinitaurBuilder.h"
#include "MinitaurController.h"
#include "StatusComponent.h"
#include <cassert>

wolf::GameObject& MinitaurBuilder::BuildMinitaur(const EnemyData& data, const glm::vec2& position)
{
    // Create the Minitaur GameObject
    wolf::GameObject* minitaurObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = minitaurObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    minitaurObject->AddComponent<HealthComponent>(data.health);
    minitaurObject->AddComponent<VelocityComponent>();
    auto& collider = minitaurObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDR, false, true);
    collider.AddColliderBox(glm::vec2(11.0f, 16.0f), glm::vec2(-6.0f, 7.0f));

    // Add status component
    auto& statusComponent = minitaurObject->AddComponent<StatusComponent>();

    // Add MinitaurController and initialize it with data
    auto& controller = minitaurObject->AddComponent<MinitaurController>();
    controller.Init(data);
    controller.SetPlayerID(m_scene.GetPlayerID());

    return *minitaurObject;
}