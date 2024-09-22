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
    auto& sprite = m_pPlayerObject->AddComponent<wolf::Sprite2D>("data/textures/sPlayerTest.png");
    sprite.SetOriginToCenterOfTexture();
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3));

    // Add Velocity and PlayerController components to the player object
    auto& velocity = m_pPlayerObject->AddComponent<VelocityComponent>();
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth builder component to an empty object
    m_pLabyrinthBuilder = &scene.CreateObject().AddComponent<LabyrinthBuilder>();
}

void PlayState::Exit()
{
    // Clean up game resources when exiting PlayState
    if (m_pPlayerObject)
    {
        m_pGameInstance->GetScene().DeleteObject(m_pPlayerObject->GetID());  // Access m_scene via GetScene()
        m_pPlayerObject = nullptr;
    }

    if (m_pLabyrinthBuilder)
    {
        // Dereference the pointer to the GameObject returned by GetGameObject() to call GetID()
        m_pGameInstance->GetScene().DeleteObject(m_pLabyrinthBuilder->GetGameObject()->GetID());
        m_pLabyrinthBuilder = nullptr;
    }
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

    // Toggle labyrinth builder gui with 'L' key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_L)) m_showLabyrinthBuilder = !m_showLabyrinthBuilder;

    // Show the Labyrinth Builder debug GUI
    if (m_showLabyrinthBuilder) m_pLabyrinthBuilder->ShowGUI();

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
}

void PlayState::Render()
{
    // Render the game's scene
    m_pGameInstance->GetScene().Render();
}

void PlayState::BackgroundUpdate(float delta)
{
    // Update logic for when state is inactive

    // Show the Labyrinth Builder debug GUI
    if (m_showLabyrinthBuilder) m_pLabyrinthBuilder->ShowGUI();

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