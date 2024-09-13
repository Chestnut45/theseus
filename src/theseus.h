#pragma once

#include <wolf.h>

class Theseus : public wolf::App
{
    // Interface
    public:

        Theseus();
        ~Theseus();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;
    
    // Data / implementation
    private:

        // The main game scene
        wolf::Scene m_scene;

        // A pointer to the player game object
        wolf::GameObject* m_pPlayerObject = nullptr;

        //manager for handling different states
        GameStateManager* m_stateManager;
};