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
    // Handle any hotkeys (such as toggling debug mode or the labyrinth builder)
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) m_showDebug = !m_showDebug;
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_L)) m_showLabyrinthBuilder = !m_showLabyrinthBuilder;

    // Handle window resizing
    if (m_windowResized)
    {
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);
        m_windowResized = false;
    }

    // Update the current game state (whether MainMenu, Play, etc.)
    if (m_pStateManager)
    {
        m_pStateManager->Update(delta);
    }

    // Show the labyrinth builder GUI if toggled
    if (m_showLabyrinthBuilder && m_pLabyrinthBuilder)
    {
        m_pLabyrinthBuilder->ShowGUI();
    }

    // Show debug information window
    if (m_showDebug)
    {
        ShowDebug();
    }
}

void Theseus::Render()
{
    // Clear the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the current game state
    if (m_pStateManager)
    {
        m_pStateManager->Render();
    }
}
