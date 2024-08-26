#include "theseus.h"

#include "W_Input.h"
#include "W_Logging.h"

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
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) Shutdown();
    
    // TODO: Update logic
}

void Theseus::Render()
{
    // TODO: Rendering logic
}