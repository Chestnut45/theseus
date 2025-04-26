#include "Theseus.h"

#include <stb_image.h>

#include <MainMenuState.h>
#include <PlayState.h>
#include <LightComponent.h>
#include <BoundedFluidSystem2D.h>

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
    // Setup window icon
    GLFWimage images[1]; 
    images[0].pixels = stbi_load("data/textures/icon.png", &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(m_pWindow, 1, images); 
    stbi_image_free(images[0].pixels);

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
    // Handle window resizing
    if (m_windowResized)
    {
        // Update camera's size to match the window
        wolf::Camera2D* camera = m_scene.GetActiveCamera();
        if (camera) camera->SetViewSize(m_width, m_height);
        
        // Resize static framebuffers
        LightComponent::ResizeFBO(m_width, m_height);
        BoundedFluidSystem2D::ResizeFramebuffer(m_width, m_height);

        // Reset flag
        m_windowResized = false;
    }

    // Update the current game state (whether MainMenu, Play, etc.)
    m_pStateManager->Update(delta);
    
    if (m_showDebugGUI) ShowDebug();
}

void Theseus::Render(float delta)
{
    // Clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the current game state
    m_pStateManager->Render(delta);
}