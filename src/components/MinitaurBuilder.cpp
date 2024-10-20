#include "MinitaurBuilder.h"
#include "MinitaurController.h"
#include <cassert>

// Build the Minitaur GameObject and initialize its components
wolf::GameObject& MinitaurBuilder::BuildMinitaur(const EnemyData& data, const glm::vec2& position, ColliderManager* pColliderManager) {
    // Create the Minitaur GameObject
    wolf::GameObject* minitaurObject = &m_scene.CreateObject2D();

    // Set the position from the constructor parameter
    auto* transform = minitaurObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(position);

    // The scale will be inherited from the Labyrinth, so no need to set it here

    // Add components using the data
    minitaurObject->AddComponent<HealthComponent>(data.health);
    minitaurObject->AddComponent<VelocityComponent>();
    auto& collider = minitaurObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add ArmourComponent
    auto& armourComponent = minitaurObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(data.armour);
    
    // add AnimatedSprite2D Component
    auto& animComponent = minitaurObject->AddComponent<AnimatedSprite2D>("data/textures/Minitaur-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);


    // Add MinitaurController and initialize it with data
    auto& controller = minitaurObject->AddComponent<MinitaurController>();
    controller.Init(data);

    return *minitaurObject;
}
