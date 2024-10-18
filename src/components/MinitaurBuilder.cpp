#include "MinitaurBuilder.h"
#include "MinitaurController.h"
#include <cassert>

// Build the Minitaur GameObject and initialize its components
wolf::GameObject& MinitaurBuilder::BuildMinitaur(const EnemyData& data, ColliderManager* pColliderManager) {
    // Create the Minitaur GameObject
    wolf::GameObject* minitaurObject = &m_scene.CreateObject2D();

    // Set initial position and scale from data
    auto* transform = minitaurObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(data.position);
    transform->SetScale(glm::vec2(data.scale));

    // Add components using the data
    minitaurObject->AddComponent<HealthComponent>(data.health);
    minitaurObject->AddComponent<VelocityComponent>();
    auto& collider = minitaurObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add ArmourComponent
    auto& armourComponent = minitaurObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(data.armour);
    auto& animComponent = minitaurObject->AddComponent<AnimatedSprite2D>("data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);


    // Add MinitaurController and initialize it with data
    auto& controller = minitaurObject->AddComponent<MinitaurController>();
    controller.Init(data);

    return *minitaurObject;
}
