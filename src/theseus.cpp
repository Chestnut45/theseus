
#include "theseus.h"

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

    // Initialize the game state manager
    m_pStateManager = new GameStateManager();
    
    // Start with the Main Menu State
    m_pStateManager->PushState(new MainMenuState(m_pStateManager, this));
}

Theseus::~Theseus()
{
    delete m_pStateManager;  // Clean up the game state manager
}

void Theseus::Update(float delta)
{
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) m_showDebug = !m_showDebug;

    // Handle window resizing
    if (m_windowResized)
    {
        // Update camera's size to match the window
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);
        m_windowResized = false;
    }

    // Update the current game state (whether MainMenu, Play, etc.)
    m_pStateManager->Update(delta);
    
    if (m_showDebug) ShowDebug();

    this->m_pHitboxManager->Update();
    this->m_pHurtboxManager->Update();
}

void Theseus::Render()
{
    // Clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the current game state
    m_pStateManager->Render();
}