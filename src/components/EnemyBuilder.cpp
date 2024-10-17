#include "EnemyBuilder.h"
#include <cassert>

// Build the enemy GameObject and initialize its components
wolf::GameObject& EnemyBuilder::BuildEnemy(ColliderManager* pColliderManager)  // Add parameter
{
    // Create the enemy GameObject
    m_pEnemyObject = &m_scene.CreateObject2D();

    // Add the EnemyController component to handle behavior and state
    m_pEnemyController = &m_pEnemyObject->AddComponent<EnemyController>(150.0f);

    // Set ColliderManager for EnemyController
    m_pEnemyController->SetColliderManager(pColliderManager);  // Pass it here

    // Set initial position
    auto* transform = m_pEnemyObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(800.0f, 500.0f));  // Example position
    transform->SetScale(glm::vec2(3.0f));

    // Add the VelocityComponent
    m_pEnemyObject->AddComponent<VelocityComponent>();

    // Add ColliderComponent
    auto& collider = m_pEnemyObject->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDD, false, true);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));  // Example box size

    // Add HealthComponent
    m_pEnemyObject->AddComponent<HealthComponent>(100);

    // Add ArmourComponent
    auto& armourComponent = m_pEnemyObject->AddComponent<ArmourComponent>();
    armourComponent.CollectArmour(2);  // Example armor

    // Initialize the EnemyController
    m_pEnemyController->Init();

    return *m_pEnemyObject;
}