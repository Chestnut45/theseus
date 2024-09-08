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

Theseus::Theseus() : App("Theseus", 1280, 720), m_camera(1280, 720)
{
    // TODO: Initialization logic
}

Theseus::~Theseus()
{
    // TODO: Shutdown logic
}

void Theseus::Update(float delta)
{
    // Debug hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) wolf::Audio::Play("data/sounds/omg.mp3");
    
    // TODO: Update logic
}

void Theseus::Render()
{
    // Clear the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // DEBUG: Sprite testing
    static wolf::Sprite2D sprite("data/textures/sPlayerTest.png");

    // Bind the camera buffer
    m_camera.Bind();

    // Draw the test sprite in world space rotating and changing scale and tint over time
    sprite.Draw({0, 0}, // Position
                sin(m_programLifetime) * 180, // Rotation
                {4, 4}, // Scale
                {sin(m_programLifetime), cos(m_programLifetime), tan(m_programLifetime)} // Color
                );

    // TODO: Rendering logic
}