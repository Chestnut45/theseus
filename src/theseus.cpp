#include "theseus.h"

#include "PlayerController.h"
#include "GameInc.h"
#include "VelocityComponent.h"

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
    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Create player object with a 2D transform component
    m_pPlayerObject = &m_scene.CreateObject2D();

    // Add a test sprite to the player object
    auto& sprite = m_pPlayerObject->AddComponent<wolf::Sprite2D>("data/textures/sPlayerTest.png");
    sprite.SetOriginToCenterOfTexture();

    // Scale up the player object's transform (affects the sprite size)
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4, 4));

    // Add the main camera as a component of the player object
    // TODO: This should make the camera follow the player game object smoothly
    // NOTE: Will implement in wolf::Scene::Update(float)
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    m_scene.SetActiveCamera(camera);
    wolf::Transform2D* pTransform = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    VelocityComponent* pVelocity = &m_pPlayerObject->AddComponent<VelocityComponent>(pTransform);
    PlayerController* playerController = &m_pPlayerObject->AddComponent<PlayerController>(pTransform, pVelocity);
    m_stateManager = new GameStateManager();  // Initialize m_stateManager

    // Set the initial state to the Main Menu
    m_stateManager->SetState(new MainMenuState(m_stateManager));
}

Theseus::~Theseus()
{
    // TODO: Shutdown logic
    delete m_stateManager;
}

void Theseus::Update(float delta)
{
    // Hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();

    // Handle the window resized flag
    if (m_windowResized)
    {
        // Update the active camera's view size
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);

        // Reset the flag
        m_windowResized = false;
    }

    // Update the player controller
    auto* playerController = m_pPlayerObject->GetComponent<PlayerController>();
    if (playerController)
    {
        playerController->Update(delta); 
    }
    if (m_stateManager)
    {
        m_stateManager->Update(delta);
    }

    // Update all components / game objects in the scene
    m_scene.Update(delta);
}

void Theseus::Render()
{
    // Clear the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the scene
    m_scene.Render();
}