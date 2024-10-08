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

    // Initialise hitbox and hurtbox managers
    this->m_pHitboxManager = new HitboxManager(&scene);
    this->m_pHurtboxManager = new HurtboxManager(&scene);

    if (!m_pHitboxManager || !m_pHurtboxManager)
        return;

    // Initialize the player object first
    CreatePlayer();

    if (!m_pPlayerObject || !m_pPlayerObject->GetComponent<PlayerController>())
        return;

    // Initialize the Minitaur object second
    CreateMinitaurEnemy();

    if (!m_pMinitaurObject)
        return;

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    camera.SetFollowSpeed(2.0f);
    scene.SetActiveCamera(camera);

    // Add the labyrinth manager component to an empty object
    m_pLabyrinthManager = &scene.CreateObject2D().AddComponent<LabyrinthManager>();

    if (!m_pLabyrinthManager)
        return;
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
    // Push the pause state when 'Escape' is pressed
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        m_pStateManager->PushState(new PauseState(m_pStateManager, m_pGameInstance));
    }

    // Toggle Labyrinth Manager GUI with the semicolon key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SEMICOLON)) 
        m_showLabyrinthManager = !m_showLabyrinthManager;

    // Show the Labyrinth Manager debug GUI
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();

    // Main object / component updates
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) 
        playerController->Update(delta);

    auto* playerAnim = m_pPlayerObject->GetComponent<AnimatedSprite2D>();
    if (playerAnim) 
        playerAnim->Update(delta);

    auto* enemyController = m_pMinitaurObject->GetComponent<EnemyController>();
    if (enemyController)
    {
        enemyController->Update(delta);
    }

    // Apply velocity to transforms for all objects with both components
    for (auto&& [_, transform, velocity] : m_pGameInstance->GetScene().Each<wolf::Transform2D, VelocityComponent>())
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
    if (m_showLabyrinthManager) 
        m_pLabyrinthManager->ShowGUI();
}

void PlayState::BackgroundRender()
{
    m_pGameInstance->GetScene().Render();
}

void PlayState::CreatePlayer()
{
    PlayerBuilder playerBuilder(m_pGameInstance->GetScene());
    m_pPlayerObject = &playerBuilder.BuildPlayer();

    auto* playerController = playerBuilder.GetPlayerController();
    if (playerController)
    {
        playerController->SetManagers(m_pHitboxManager, m_pHurtboxManager);
    }
}

void PlayState::CreateMinitaurEnemy()
{
    EnemyBuilder enemyBuilder(m_pGameInstance->GetScene());
    m_pMinitaurObject = &enemyBuilder.BuildEnemy();

    auto* enemyController = enemyBuilder.GetEnemyController();
    if (!enemyController)
    {
        return;
    }
}
