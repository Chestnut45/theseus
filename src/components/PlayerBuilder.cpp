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

    // Add the PlayerController component, which will handle all animation initialization
    m_pPlayerController = &m_pPlayerObject->AddComponent<PlayerController>();

    // Check if PlayerController was added successfully
    assert(m_pPlayerController != nullptr && "Failed to add PlayerController component");

    // Scale the player
    auto* transform = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    assert(transform != nullptr && "Failed to retrieve Transform2D component for the player");

    if (transform)
    {
        transform->SetScale(glm::vec2(3));
    }

    // Add other components as needed
    m_pPlayerObject->AddComponent<VelocityComponent>();
    auto& hitbox = m_pPlayerObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    auto& hurtbox = m_pPlayerObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    m_pPlayerObject->AddComponent<HealthComponent>();
    m_pPlayerObject->AddComponent<ArmourComponent>(50);

    // Call LateInitialize() on PlayerController after GameObject is fully created and registered
    m_pPlayerController->LateInitialize();

    return *m_pPlayerObject;
}

// Function to get the PlayerController
PlayerController* PlayerBuilder::GetPlayerController() const
{
    return m_pPlayerController;
}