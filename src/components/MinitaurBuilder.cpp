#include "MinitaurBuilder.h"
#include "MinitaurController.h"
#include <cassert>

// Build the Minitaur GameObject and initialize its components
wolf::GameObject& MinitaurBuilder::BuildMinitaur(ColliderManager* pColliderManager)
{
    // Create the Minitaur GameObject
    m_pMinitaurObject = &m_scene.CreateObject2D();

    // Add the MinitaurController component to handle behavior and state
    m_pMinitaurController = &m_pMinitaurObject->AddComponent<MinitaurController>();

    // Set initial position for the Minitaur
    auto* transform = m_pMinitaurObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(800.0f, 500.0f));  // Example starting position
    transform->SetScale(glm::vec2(3.0f));  // Scale the Minitaur

    // Add the VelocityComponent for movement
    m_pMinitaurObject->AddComponent<VelocityComponent>();

    // Add the ColliderComponent for collisions
    auto& collider = m_pMinitaurObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));  // Example box size

    // Add the HealthComponent for health management
    m_pMinitaurObject->AddComponent<HealthComponent>(150);  // Example health value

    // Add ArmourComponent for armor (optional, adjust as needed)
    auto& armourComponent = m_pMinitaurObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(3);  // Example armor value

    // Initialize the MinitaurController
    m_pMinitaurController->Init();

    return *m_pMinitaurObject;
}
