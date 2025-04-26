//-----------------------------------------------------------------------------
// File: SnakeBuilder.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật, D'Anyil Landry
// Constructs snake enemies.
//-----------------------------------------------------------------------------

#include "SnakeBuilder.h"
#include "StatusComponent.h"
#include <cassert>
#include <LightComponent.h>

wolf::GameObject& SnakeBuilder::BuildSnake(const EnemyData& data, const glm::vec2& position)
{
    // Create the snake GameObject
    wolf::GameObject* snakeObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = snakeObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    snakeObject->AddComponent<HealthComponent>(data.health);
    snakeObject->AddComponent<VelocityComponent>();
    auto& collider = snakeObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDR, false, true);
    collider.AddColliderBox(glm::vec2(8.0f, 8.0f), glm::vec2(-4.0f, 4.0f));

    // Add status component
    auto& statusComponent = snakeObject->AddComponent<StatusComponent>();

    // Add SnakeController and initialize it with data
    auto& controller = snakeObject->AddComponent<SnakeController>();
    controller.Init(data);
    controller.SetPlayerID(m_scene.GetPlayerID());

    // Add a light to the snake
    wolf::GameObject* pLightGO = &m_scene.CreateObject2D();
    auto& pLightComponent = pLightGO->AddComponent<LightComponent>(glm::vec4(0.45f, 0.9f, 0.64f, 0.65f), 32.0f, true);
    snakeObject->AddChild(*pLightGO);
    pLightComponent.Init();
    pLightComponent.SetShadowsEnabled(false);

    return *snakeObject;
}