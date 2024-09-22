#pragma once

#include <wolf.h>
#include "GameInc.h"
#include "LabyrinthBuilder.h"
#include "PlayerController.h"
#include "VelocityComponent.h"

class Theseus : public wolf::App
{
public:
    Theseus();
    ~Theseus();

    // Update the app, called every frame
    void Update(float delta) override;
    
    // Rendering logic, called every frame
    void Render() override;

    // Cleanly shut down the game
    // NOTE: This will trigger the App's destructor, so
    // make sure to cleanup your resources before shutting down.
    inline void Shutdown() { glfwSetWindowShouldClose(m_pWindow, true); }

    // Access to the game's main scene
    inline wolf::Scene& GetScene() { return m_scene; }

private:

    // Main game scene
    wolf::Scene m_scene;

    // Manager for handling game states
    GameStateManager* m_pStateManager = nullptr;

    // Flags
    bool m_showDebug = false;
};
