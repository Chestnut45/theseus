#include "theseus.h"

#include <wolf.h>

// Application entrypoint
int main(int, char**)
{
    Theseus app;
    app.Run();
    return 0;
}

Theseus::Theseus() : App("Theseus", 1280, 720)
{
    // TODO: Initialization logic
}

Theseus::~Theseus()
{
    // TODO: Shutdown logic
}

void Theseus::Update(float delta)
{
    // Close game with escape key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();

    // Show debug with tilde key
    if (wolf::Input::IsKeyDown(GLFW_KEY_GRAVE_ACCENT)) ShowDebug();
    
    // TODO: Update logic
}

void Theseus::Render()
{
    // Clear the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // TODO: Rendering logic
}