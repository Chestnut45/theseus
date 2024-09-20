#pragma once

#include <wolf.h>
#include "GameInc.h"
#include "LabyrinthBuilder.h"
#include "PlayerController.h"
#include "VelocityComponent.h"
#include "GameStateManager.h"
#include "MainMenuState.h"

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
    void Shutdown() { glfwSetWindowShouldClose(m_pWindow, true); }
    wolf::Scene& GetScene() { return m_scene; }

private:
    // The main game scene
    wolf::Scene m_scene;

    // A pointer to the player game object
    wolf::GameObject* m_pPlayerObject = nullptr;

    // A pointer to the labyrinth builder component
    LabyrinthBuilder* m_pLabyrinthBuilder = nullptr;

    // Manager for handling game states
    GameStateManager* m_pStateManager = nullptr;

    // Flags
    bool m_showDebug = false;
    bool m_showLabyrinthBuilder = false;
};
