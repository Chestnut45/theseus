#include "EnemyBuilder.h"
#include <cassert>

// Build the enemy GameObject and initialize its components
wolf::GameObject& EnemyBuilder::BuildEnemy()
{
    // Create the enemy GameObject
    m_pEnemyObject = &m_scene.CreateObject2D();

    // Verify that the GameObject was created successfully
    m_pEnemyController = &m_pEnemyObject->AddComponent<EnemyController>(150.0f);

    // Set initial position away from the player (example position)
    auto* transform = m_pEnemyObject->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(800.0f, 500.0f));  // Set position away from the player for testing
    transform->SetScale(glm::vec2(3.0f));

    // Remove the AnimatedSprite2D initialization from here.
    // The EnemyController will handle the AnimatedSprite2D setup.

    // Add the EnemyController component to handle behavior and state

    // Add necessary components to the enemy
    m_pEnemyObject->AddComponent<VelocityComponent>();

    auto& hitbox = m_pEnemyObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    auto& hurtbox = m_pEnemyObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    m_pEnemyObject->AddComponent<HealthComponent>(100);
    m_pEnemyObject->AddComponent<ArmourComponent>(50);

    // Initialize the EnemyController after all components are added
    m_pEnemyController->Init();

    return *m_pEnemyObject;
}