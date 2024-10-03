#include "PlayState.h"
#include "PauseState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HitboxComponent.h"
#include "../components/HurtboxComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/PlayerBuilder.h"

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();

    // Initialise hitbox / hurtbox managers
    this->m_pHitboxManager = new HitboxManager(&scene);
    this->m_pHurtboxManager = new HurtboxManager(&scene);

    // Initialize player object first
    CreatePlayer();

    // Check if the player has been correctly initialized
    if (!m_pPlayerObject->GetComponent<PlayerController>())
    {
        std::cout << "Error: PlayerController not found in player object!" << std::endl;
    }

    // Initialize Minitaur object second
    CreateMinitaurEnemy();

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager component to an empty object
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();

    // TESTING BELOW

    // Create test projectile object
    auto& testObj = scene.CreateObject2D();
    testObj.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));
    testObj.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(256.0f, 0.0f));
    auto& testSprite = testObj.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
    auto& testHitbox = testObj.AddComponent<HitboxComponent>(1, 1);
    testHitbox.AddHitbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testHurtbox = testObj.AddComponent<HurtboxComponent>(1, 1, 1, 1);
    testHurtbox.AddHurtbox(glm::vec2(32.0f, 32.0f), glm::vec2(0.0f, 0.0f));
    auto& testVelocity = testObj.AddComponent<VelocityComponent>();
    testVelocity.SetVelocity(glm::vec2(-32.0f, 0.0f));
}

void PlayState::Exit()
{
    // Delete objects / components from the scene
    m_pGameInstance->GetScene().Clear();

    // Delete managers
    delete this->m_pHitboxManager;
    this->m_pHitboxManager = nullptr;
    delete this->m_pHurtboxManager;
    this->m_pHurtboxManager = nullptr;
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

    // Toggle Labyrinth Manager GUI with the semicolon key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SEMICOLON)) m_showLabyrinthManager = !m_showLabyrinthManager;

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) m_pLabyrinthManager->ShowGUI();

    // Main object / component updates

    // Update player controller
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) playerController->Update(delta);

    // Update player animations
    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) {
        playerAnim->Update(delta);
    }

    auto* enemyController = m_pMinitaurObject->GetComponent<EnemyController>();
    if (enemyController)
    {
        enemyController->Update(delta);
        auto* velocityComponent = m_pMinitaurObject->GetComponent<VelocityComponent>();
        if (velocityComponent)
        {
            glm::vec2 velocity = velocityComponent->GetVelocity();
            // std::cout << "Minitaur Velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
        }
    }

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
    // Create a PlayerBuilder and build the player GameObject
    PlayerBuilder playerBuilder(m_pGameInstance->GetScene());
    m_pPlayerObject = &playerBuilder.BuildPlayer();

    // Set the Hitbox and Hurtbox managers in the PlayerController
    auto* playerController = playerBuilder.GetPlayerController();
    if (playerController)
    {
        playerController->SetManagers(m_pHitboxManager, m_pHurtboxManager);
    }
    else
    {
        std::cerr << "Error: Failed to create and initialize player controller!" << std::endl;
    }
}

void PlayState::CreateMinitaurEnemy()
{
    // Create Minitaur object with transform
    m_pMinitaurObject = &m_pGameInstance->GetScene().CreateObject2D();

    // Add EnemyController to the Minitaur object
    auto& enemyController = m_pMinitaurObject->AddComponent<EnemyController>(150.0f); // Initialize with chase speed

    // Scale and position the Minitaur
    auto* transform = m_pMinitaurObject->GetComponent<wolf::Transform2D>();
    transform->SetScale(glm::vec2(3));                         // Scale the Minitaur
    transform->SetPosition(glm::vec2(500.0f, 500.0f));         // Set the initial position, ensure this is valid

    // Add a sprite component for the Minitaur
    auto& sprite = m_pMinitaurObject->AddComponent<wolf::Sprite2D>("data/textures/minitaur.png");
    sprite.SetOriginToCenterOfTexture(); // Optional: center the sprite to the transform origin

    // Check sprite and transform validity
    if (!sprite.GetTexture()) 
    {
        std::cerr << "Error: Minitaur sprite texture not loaded correctly!" << std::endl;
    }
    std::cout << "Minitaur Initial Position: " << transform->GetGlobalPosition().x << ", " << transform->GetGlobalPosition().y << std::endl;

    // Add velocity component for movement
    m_pMinitaurObject->AddComponent<VelocityComponent>();

    // Add hitbox component
    auto& hitbox = m_pMinitaurObject->AddComponent<HitboxComponent>(0, 1);
    hitbox.AddHitbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add hurtbox component
    auto& hurtbox = m_pMinitaurObject->AddComponent<HurtboxComponent>(0, 0, 0, 1);
    hurtbox.AddHurtbox(glm::vec2(13.0f, 26.0f), glm::vec2(-7.0f, -14.0f));

    // Add health and armor components
    m_pMinitaurObject->AddComponent<HealthComponent>(100);
    m_pMinitaurObject->AddComponent<ArmourComponent>(50);

    // Now, explicitly call the Init() method on the EnemyController
    enemyController.Init();

    std::cout << "Minitaur successfully created and initialized!" << std::endl;
}
