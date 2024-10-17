#include "EnemyBuilder.h"
#include <cassert>

// Build the enemy GameObject and initialize its components
wolf::GameObject& EnemyBuilder::BuildEnemy()
{
    // Create the enemy GameObject
    m_pEnemyObject = &m_scene.CreateObject2D();

    // Add the EnemyController component to handle behavior and state
    m_pEnemyController = &m_pEnemyObject->AddComponent<EnemyController>(150.0f);

    // Set initial position away from the player (example position)
    auto* transform = m_pEnemyObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(800.0f, 500.0f));  // Set position away from the player for testing
    transform->SetScale(glm::vec2(3.0f));

    // Add the VelocityComponent to handle enemy movement
    m_pEnemyObject->AddComponent<VelocityComponent>();

    // Add the ColliderComponent and configure it
    auto& collider = m_pEnemyObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    
    // Configure the collider box (this replaces the hitbox and hurtbox)
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));  // Example dimensions and offset

    // Add the HealthComponent
    m_pEnemyObject->AddComponent<HealthComponent>(100);   // Health component with 100 HP

    // Add the ArmourComponent using the default constructor and then configure it
    auto& armourComponent = m_pEnemyObject->AddComponent<ArmourComponent>();
    
    // Set armour multiplier and any special properties using the CollectArmour method
    armourComponent.CollectArmour(2);  // Set multiplier to 50, adjust if needed

    // Initialize the EnemyController after all components are added
    m_pEnemyController->Init();

    return *m_pEnemyObject;
}