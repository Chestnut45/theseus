#include "theseus.h"

#include "PlayerController.h"
#include "GameInc.h"
#include "VelocityComponent.h"
#include "MainMenuState.h"
#include "GameStateManager.h"

// Application entrypoint
int main(int, char**)
{
    // Unit tests
    wolf::_SceneTests();
    wolf::_ShapeTests();
    wolf::_EventManagerTests();
    
    Theseus app;
    app.Run();
    return 0;
}

Theseus::Theseus() : App("Theseus", 1280, 720)
{
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Initialize the game state manager and set the initial state to Main Menu
    m_pStateManager = new GameStateManager();
    m_pStateManager->SetState(new MainMenuState(m_pStateManager));

    // Create player object
    m_pPlayerObject = &m_scene.CreateObject2D();

    // Add a test sprite to the player object and scale up
    auto& sprite = m_pPlayerObject->AddComponent<wolf::Sprite2D>("data/textures/sPlayerTest.png");
    sprite.SetOriginToCenterOfTexture();
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4));

    // Add Velocity and PlayerController components to the player object
    auto& pVelocity = m_pPlayerObject->AddComponent<VelocityComponent>();
    auto& playerController = m_pPlayerObject->AddComponent<PlayerController>();

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    m_scene.SetActiveCamera(camera);

    // Add a test tilemap
    auto& tileMapObject = m_scene.CreateObject2D();
    auto& tileMap = tileMapObject.AddComponent<wolf::TileMap>(64, 64);
    tileMapObject.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4));
    tileMap.LoadTileSet("data/labyrinth.tileset");
    tileMap.SetTile(0, 0, 5);
    tileMap.SetTile(1, 0, 6);
    tileMap.SetTile(0, 1, 2);
}

Theseus::~Theseus()
{
    delete m_pStateManager; // Clean up state manager
}

void Theseus::Update(float delta)
{
    // Hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();

    // Handle window resizing
    if (m_windowResized)
    {
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);
        m_windowResized = false;
    }

    // Game systems

    // Update the current game state (MainMenu, Play, etc.)
    if (m_pStateManager) m_pStateManager->Update(delta);

    // Update the player controller
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController) playerController->Update(delta);

    // Apply velocity to all transform components
    for (auto&& [id, transform, velocity] : m_scene.Each<wolf::Transform2D, VelocityComponent>())
    {
        transform.Translate(velocity.GetVelocity() * delta);
    }

    // Update all game objects and components the scene handles automatically
    m_scene.Update(delta);
}

void Theseus::Render()
{
    // Clear the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the game scene
    m_scene.Render();
}
