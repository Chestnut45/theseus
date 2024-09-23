#include "PlayState.h"
#include "PauseState.h"
#include <imgui/imgui.h>

void PlayState::Enter()
{
    // Initialize the game world, player, and camera when PlayState is entered
    auto& scene = m_pGameInstance->GetScene();  // Access m_scene via GetScene()

    // Create player object
    m_pPlayerObject = &scene.CreateObject2D();

    // Add a sprite to the player object and scale it up
    auto& animSprite = m_pPlayerObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
    animSprite.AddAnimation("WalkSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, true);
    animSprite.AddAnimation("WalkEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, true);
    animSprite.AddAnimation("WalkNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, true);
    animSprite.AddAnimation("WalkWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, true);
    animSprite.AddAnimation("StandSouth", "data/textures/TheseusStand-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    animSprite.SetAnimation("WalkSouth");
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add Velocity and PlayerController components to the player object
    auto& velocity = m_pPlayerObject->AddComponent<VelocityComponent>();
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();

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

    // Update player animations
    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) {
        playerAnim->Update(delta);
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
            playerAnim->SetAnimation("StandSouth");
        }
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_Q)) {
            playerAnim->SetAnimation("WalkSouth");
        }
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