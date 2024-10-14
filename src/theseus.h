#pragma once

#include <wolf.h>
#include "GameInc.h"
#include "DialogueManager.h"
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

    // Accessors

    // Gets the dimensions of the game window
    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }

    // Gets a reference to the main scene of the game
    inline wolf::Scene& GetScene() { return m_scene; }
    DialogueManager& GetDialogueManager() { return m_dialogueManager; }


private:

    // Main game scene
    wolf::Scene m_scene;

    // Manager for handling game states
    GameStateManager* m_pStateManager = nullptr;
    DialogueManager m_dialogueManager;

    // Flags
    bool m_showDebug = false;
};
