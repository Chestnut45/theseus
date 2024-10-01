#include "PlayState.h"
#include "PauseState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/ColliderComponent.h"
#include "../components/HealthComponent.h"
#include "../components/StatusComponent.h"
#include "../components/VelocityComponent.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialise managers
    this->m_pColliderManager = new ColliderManager(&scene);
    this->m_pStatusManager = new StatusManager(&scene);

    // Initialize player object
    CreatePlayer();

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager component to an empty object
    m_pLabyrinthManager = &scene.CreateObject().AddComponent<LabyrinthManager>();

    // TESTING BELOW

    // Create test object
    auto& testObj = scene.CreateObject2D();
    testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    testObj.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(512.0f, 0.0f));
    auto& testSprite = testObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    auto& testCollider = testObj.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
    testCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testVelocity = testObj.AddComponent<VelocityComponent>();
    testVelocity.SetVelocity(glm::vec2(-64.0f, 0.0f));


    auto& testObj2 = scene.CreateObject2D();
    testObj2.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
    testObj2.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(256.0f, 0.0f));
    auto& testSprite2 = testObj2.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    auto& testCollider2 = testObj2.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDD, 0, 1);
    testCollider2.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testVelocity2 = testObj2.AddComponent<VelocityComponent>();
    testVelocity2.SetVelocity(glm::vec2(64.0f, 0.0f));

}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    // Delete managers
    delete this->m_pColliderManager;
    this->m_pColliderManager = nullptr;

    delete this->m_pStatusManager;
    this->m_pStatusManager = nullptr;
}

void PlayState::Pause()
{
}

void PlayState::Resume()
{
}

void PlayState::Update(float delta)
{
    // Input and debug hotkey handling

    // Push the pause state when 'Escape' is pressed
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        m_pStateManager->PushState(new PauseState(m_pStateManager, m_pGameInstance));
    }

    // Toggle Labyrinth Manager GUI with 'L' key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_L)) m_showLabyrinthManager = !m_showLabyrinthManager;

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();

    // Main object / component updates

    // Update player controller
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) playerController->Update(delta);

    // Update managers
    this->m_pColliderManager->Update(delta);
    this->m_pStatusManager->Update();

    // Update player animations
    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) {
        playerAnim->Update(delta);
    }

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())  // Use GetScene()
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }   

    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);


}

void PlayState::Render()
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController)
        playerController->Render();
}

void PlayState::BackgroundUpdate(float delta)
{
    // Update logic for when state is inactive

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();
}

void PlayState::BackgroundRender()
{
    // Render logic for when state is inactive
    
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
}

void PlayState::CreatePlayer()
{
    // Create player object with transform
    m_pPlayerObject = &m_pGameInstance->GetScene().CreateObject2D();

    // Add player controller
    m_pPlayerObject->AddComponent<PlayerController>();

    // Scale player
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add animated sprite
    auto& animSprite = m_pPlayerObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
    animSprite.AddAnimation("WalkSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, true);
    animSprite.AddAnimation("WalkEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, true);
    animSprite.AddAnimation("WalkNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, true);
    animSprite.AddAnimation("WalkWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, true);
    animSprite.AddAnimation("StandSouth", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    animSprite.AddAnimation("StandEast", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 2, 2, false);
    animSprite.AddAnimation("StandNorth", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 3, 3, false);
    animSprite.AddAnimation("StandWest", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 4, 4, false);
    animSprite.SetAnimation("StandSouth");
    animSprite.SetOriginToCenterOfFrame();

    // Add velocity
    m_pPlayerObject->AddComponent<VelocityComponent>();

    // Add collider
    auto& collider = m_pPlayerObject->AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HITHURTBOXDR, 0, 1);
    collider.AddColliderBox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));
   
    // Add status
    auto& status = m_pPlayerObject->AddComponent<StatusComponent>();
    // status.AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 3);
    // status.AddStatusEffect(StatusComponent::StatusEffectType::POISONED, 3);
    // status.AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 3);

    // Add health / armor
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>(1000);
    auto& armour = m_pPlayerObject->AddComponent<ArmourComponent>();
    armour.CollectArmour(50, {{ArmourComponent::SpecialProperty::FIRERESISTANCE, 50}});
}