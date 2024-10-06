#include "PlayerBuilder.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "HitboxComponent.h"
#include "HurtboxComponent.h"

PlayerBuilder::PlayerBuilder(wolf::Scene& scene)
    : m_scene(scene), m_pPlayerObject(nullptr), m_pPlayerController(nullptr) {}

// Build the player GameObject and initialize its components
wolf::GameObject& PlayerBuilder::BuildPlayer()
{
    // Create the player GameObject
    m_pPlayerObject = &m_scene.CreateObject2D();

    // Check if GameObject was created successfully
    assert(m_pPlayerObject != nullptr && "Failed to create player GameObject");
    if (!m_pPlayerObject)
    {
        std::cerr << "Error: Failed to create player GameObject!" << std::endl;
        return *m_pPlayerObject;
    }

    // Add the PlayerController component, which will handle all animation initialization
    m_pPlayerController = &m_pPlayerObject->AddComponent<PlayerController>();

    // Check if PlayerController was added successfully
    assert(m_pPlayerController != nullptr && "Failed to add PlayerController component");
    if (!m_pPlayerController)
    {
        std::cerr << "Error: Failed to add PlayerController component!" << std::endl;
        return *m_pPlayerObject;
    }

    // Scale the player
    auto* transform = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    assert(transform != nullptr && "Failed to retrieve Transform2D component for the player");
    if (transform)
    {
        transform->SetScale(glm::vec2(3));
    }
    else
    {
        std::cerr << "Error: Transform2D component not found for Player object!" << std::endl;
    }

    // Add other components as needed
    m_pPlayerObject->AddComponent<VelocityComponent>();
    std::cout << "VelocityComponent added to Player object." << std::endl;

    auto& hitbox = m_pPlayerObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));
    std::cout << "HitboxComponent added to Player object." << std::endl;

    auto& hurtbox = m_pPlayerObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));
    std::cout << "HurtboxComponent added to Player object." << std::endl;

    m_pPlayerObject->AddComponent<HealthComponent>();
    std::cout << "HealthComponent added to Player object." << std::endl;

    m_pPlayerObject->AddComponent<ArmourComponent>(50);
    std::cout << "ArmourComponent added to Player object." << std::endl;

    // Call LateInitialize() on PlayerController after GameObject is fully created and registered
    m_pPlayerController->LateInitialize();
    std::cout << "PlayerController LateInitialize() called successfully." << std::endl;

    return *m_pPlayerObject;
}

// Function to get the PlayerController
PlayerController* PlayerBuilder::GetPlayerController() const
{
    return m_pPlayerController;
}
