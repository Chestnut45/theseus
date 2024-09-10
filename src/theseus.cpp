#include "theseus.h"

// Application entrypoint
int main(int, char**)
{
    // Unit tests
    // wolf::_SceneTests();
    // wolf::_ShapeTests();
    // wolf::_EventManagerTests();
    
    Theseus app;
    app.Run();
    return 0;
}

Theseus::Theseus() : App("Theseus", 1280, 720)
{
    // TODO: Initialization logic

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Create test player object
    m_pPlayerObject = &m_scene.CreateObject2D();

    // Add the test sprite to the player object
    auto& sprite = m_pPlayerObject->AddComponent<wolf::Sprite2D>("data/textures/sPlayerTest.png");
    sprite.SetOriginToCenterOfTexture();
    m_pPlayerObject->GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(4, 4));

    // Add the main camera as a component of the player object
    auto& camera = m_pPlayerObject->AddComponent<wolf::Camera2D>(1280, 720);
    m_scene.SetActiveCamera(camera);
}

Theseus::~Theseus()
{
    // TODO: Shutdown logic
}

void Theseus::Update(float delta)
{
    // Hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) wolf::Audio::Play("data/sounds/omg.mp3");

    // Handle the window resized flag
    if (m_windowResized)
    {
        // Update the active camera's view size
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);

        // Reset the flag
        m_windowResized = false;
    }

    // Test transform hierarchy
    wolf::Transform2D* t = m_pPlayerObject->GetComponent<wolf::Transform2D>();
    if (t)
    {
        // Rotate the player's transform
        t->RotateDegrees(-180 * delta);

        // Debug player movement
        float moveSpeed = 256;
        if (wolf::Input::IsKeyDown(GLFW_KEY_W)) t->Translate(glm::vec2(0, delta * moveSpeed));
        if (wolf::Input::IsKeyDown(GLFW_KEY_A)) t->Translate(glm::vec2(-delta * moveSpeed, 0));
        if (wolf::Input::IsKeyDown(GLFW_KEY_S)) t->Translate(glm::vec2(0, -delta * moveSpeed));
        if (wolf::Input::IsKeyDown(GLFW_KEY_D)) t->Translate(glm::vec2(delta * moveSpeed, 0));
    }
    
    // TODO: Update logic
}

void Theseus::Render()
{
    // Clear the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the scene
    m_scene.Render();

    // TODO: Rendering logic
}