#pragma once

#include <wolf.h>
#include <SharedContext.h>
#include <GameStateManager.h>

class Theseus : public wolf::App
{
public:
    Theseus();
    ~Theseus();

    // Update the app, called every frame
    void Update(float delta) override;
    
    // Rendering logic, called every frame
    void Render(float delta) override;

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

    void ToggleDebugGUI() { m_showDebugGUI = !m_showDebugGUI; }
    bool IsDebugGUIEnabled() const { return m_showDebugGUI; }

    void SetDebugMode(bool value) { m_debugMode = value; }
    bool IsDebugMode() const { return m_debugMode; }

    SharedContext& GetSharedContext() { return m_sharedContext; }


private:

    // Main game scene
    wolf::Scene m_scene;

    // Manager for handling game states
    GameStateManager* m_pStateManager = nullptr;

    // Flags
    bool m_showDebugGUI = false;
    bool m_debugMode = false;


    SharedContext m_sharedContext;
};
