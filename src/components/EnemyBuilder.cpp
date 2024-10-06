#include "EnemyBuilder.h"
#include <iostream>
#include <cassert>

// Build the enemy GameObject and initialize its components
wolf::GameObject& EnemyBuilder::BuildEnemy()
{
    // Create the enemy GameObject
    m_pEnemyObject = &m_scene.CreateObject2D();

    // Verify that the GameObject was created successfully
    assert(m_pEnemyObject != nullptr && "Failed to create enemy GameObject");
    if (!m_pEnemyObject)
    {
        std::cerr << "Error: Failed to create enemy GameObject!" << std::endl;
        return *m_pEnemyObject;
    }

    // Set initial position away from the player (example position)
    auto* transform = m_pEnemyObject->GetComponent<wolf::Transform2D>();
    assert(transform != nullptr && "Failed to retrieve Transform2D component for the enemy");
    transform->SetPosition(glm::vec2(800.0f, 500.0f));  // Set position away from the player for testing
    transform->SetScale(glm::vec2(3.0f));

    // Add and configure the sprite component
    auto& sprite = m_pEnemyObject->AddComponent<wolf::Sprite2D>("data/textures/minitaur.png");
    assert(sprite.GetTexture() != nullptr && "Failed to load texture for enemy sprite");
    sprite.SetOriginToCenterOfTexture();
    std::cout << "Sprite component added and configured." << std::endl;

    // Add the EnemyController component to handle behavior and state
    m_pEnemyController = &m_pEnemyObject->AddComponent<EnemyController>(150.0f);
    assert(m_pEnemyController != nullptr && "Failed to add EnemyController component");
    if (!m_pEnemyController)
    {
        std::cerr << "Error: Failed to add EnemyController component!" << std::endl;
        return *m_pEnemyObject;
    }

    // Add necessary components to the enemy
    m_pEnemyObject->AddComponent<VelocityComponent>();

    auto& hitbox = m_pEnemyObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));
    std::cout << "Hitbox component added." << std::endl;

    auto& hurtbox = m_pEnemyObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));
    std::cout << "Hurtbox component added." << std::endl;

    m_pEnemyObject->AddComponent<HealthComponent>(100);
    m_pEnemyObject->AddComponent<ArmourComponent>(50);
    std::cout << "Health and Armour components added." << std::endl;

    // Initialize the EnemyController after all components are added
    m_pEnemyController->Init();
    std::cout << "EnemyController initialized successfully." << std::endl;

    return *m_pEnemyObject;
}
