#include "PlayState.h"
#include "PauseState.h"
#include <imgui/imgui.h>

#include "../components/ArmourComponent.h"
#include "../components/HealthComponent.h"
#include "../components/HitboxComponent.h"
#include "../components/HurtboxComponent.h"
#include "../components/VelocityComponent.h"
#include <PlayerBuilder.h>
#include <EnemyBuilder.h>

void PlayState::Enter()
{
    // Grab a reference to the main scene
    auto& scene = m_pGameInstance->GetScene();
    std::cout << "Entering PlayState: Scene reference obtained successfully." << std::endl;

    // Initialise hitbox and hurtbox managers
    this->m_pHitboxManager = new HitboxManager(&scene);
    this->m_pHurtboxManager = new HurtboxManager(&scene);
    
    // Verify if managers were created successfully
    if (!m_pHitboxManager || !m_pHurtboxManager)
    {
        std::cerr << "Error: Failed to initialize HitboxManager or HurtboxManager!" << std::endl;
        return;
    }
    std::cout << "Hitbox and Hurtbox managers initialized successfully." << std::endl;

    // Initialize the player object first
    std::cout << "Creating Player object..." << std::endl;
    CreatePlayer();

    // Check if the player has been correctly initialized
    if (!m_pPlayerObject)
    {
        std::cerr << "Error: Failed to create Player object!" << std::endl;
        return;
    }
    std::cout << "Player object created successfully." << std::endl;

    if (!m_pPlayerObject->GetComponent<PlayerController>())
    {
        std::cerr << "Error: PlayerController not found in player object!" << std::endl;
        return;
    }
    std::cout << "PlayerController component found in Player object." << std::endl;

    // Initialize the Minitaur object second
    std::cout << "Creating Minitaur enemy object..." << std::endl;
    CreateMinitaurEnemy();

    // Check if Minitaur has been created successfully
    if (!m_pMinitaurObject)
    {
        std::cerr << "Error: Failed to create Minitaur object!" << std::endl;
        return;
    }
    std::cout << "Minitaur enemy object created successfully." << std::endl;

    // Add the main camera as a component of the player object
    std::cout << "Adding Camera2D to Player object..." << std::endl;
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);
    std::cout << "Camera2D added and set as active camera." << std::endl;

    // Add the labyrinth manager component to an empty object
    std::cout << "Creating and adding LabyrinthManager component..." << std::endl;
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();

    if (!m_pLabyrinthManager)
    {
        std::cerr << "Error: Failed to create LabyrinthManager component!" << std::endl;
        return;
    }
    std::cout << "LabyrinthManager component created successfully." << std::endl;

    std::cout << "PlayState entry complete." << std::endl;
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
    std::cout << "Starting Minitaur creation..." << std::endl;

    // Use EnemyBuilder to construct the Minitaur enemy
    EnemyBuilder enemyBuilder(m_pGameInstance->GetScene());
    m_pMinitaurObject = &enemyBuilder.BuildEnemy();

    // Set the Minitaur's EnemyController if needed for additional configuration
    auto* enemyController = enemyBuilder.GetEnemyController();
    if (!enemyController)
    {
        std::cerr << "Error: Failed to create and initialize Minitaur enemy!" << std::endl;
    }

    std::cout << "Minitaur created and initialized successfully!" << std::endl;
}