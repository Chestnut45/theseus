#include "PlayState.h"
#include "PauseState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HitboxComponent.h"
#include "../components/HurtboxComponent.h"
#include "../components/VelocityComponent.h"

void PlayState::Enter()
{
    // Initialize the game world, player, and camera when PlayState is entered
    auto& scene = m_pGameInstance->GetScene();  // Access m_scene via GetScene()

    // Initialise hitbox manager
    this->m_pHitboxManager = new HitboxManager(&scene);

    // Initialise hurtbox manager
    this->m_pHurtboxManager = new HurtboxManager(&scene);  

    // Create player object
    m_pPlayerObject = &scene.CreateObject2D();

    // Add a sprite to the player object and scale it up
    auto& sprite = m_pPlayerObject->AddComponent<wolf::Sprite2D>("data/textures/sPlayerTest.png");
    sprite.SetOriginToCenterOfTexture();
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add Velocity and PlayerController components to the player object
    auto& velocity = m_pPlayerObject->AddComponent<VelocityComponent>();
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();

    // Add Hitbox component to player object
    auto& hitbox = m_pPlayerObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add Hurtbox component to player object
    auto& hurtbox = m_pPlayerObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add Health component to player object
    auto& health = m_pPlayerObject->AddComponent<HealthComponent>();

    // Add Armour component to player object
    auto& armour = m_pPlayerObject->AddComponent<ArmourComponent>(50);

    // Create test object
    auto testObj = &scene.CreateObject2D();
    testObj->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));
    testObj->GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(256.0f, 0.0f));
    auto& testSprite = testObj->AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    auto& testHitbox = testObj->AddComponent<HitboxComponent>(1, 1);
    testHitbox.AddHitbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testHurtbox = testObj->AddComponent<HurtboxComponent>(1, 1, 1, 1);
    testHurtbox.AddHurtbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testVelocity = testObj->AddComponent<VelocityComponent>();
    testVelocity.SetVelocity(glm::vec2(-32.0f, 0.0f));
    

    

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager component to an empty object
    m_pLabyrinthManager = &scene.CreateObject().AddComponent<LabyrinthManager>();
}

void PlayState::Exit()
{
    // Delete the game resources from the scene on exit
    m_pPlayerObject->Delete();

    delete this->m_pHitboxManager;
    this->m_pHitboxManager = nullptr;

    delete this->m_pHurtboxManager;
    this->m_pHurtboxManager = nullptr;

    m_pLabyrinthManager->GetGameObject()->Delete();
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

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())  // Use GetScene()
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }

    // Base update for all game objects and components in the scene
    m_pGameInstance->GetScene().Update(delta);

    // Update managers
    this->m_pHitboxManager->Update();
    this->m_pHurtboxManager->Update();
}

void PlayState::Render()
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
}

void PlayState::BackgroundUpdate(float delta)
{
    // Update logic for when state is inactive

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();

    // TODO: A small amount of updates may need to happen here (when paused)
    // NOTE: Depends on wolf::Scene::Update(...) which is changing soon
    // ASSIGNEE: D'Anyil
}

void PlayState::BackgroundRender()
{
    // Render logic for when state is inactive
    
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
}