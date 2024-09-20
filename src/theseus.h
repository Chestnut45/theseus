
#pragma once

#include <wolf.h>

#include "GameInc.h"
#include "LabyrinthBuilder.h"
#include "PlayerController.h"
#include "VelocityComponent.h"

#include "ArmourComponent.h"
#include "HealthComponent.h"
#include "HitboxComponent.h"
#include "HurtboxComponent.h"

#include "HitboxManager.h"
#include "HurtboxManager.h"

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
    
    private:

        // The main game scene
        wolf::Scene m_scene;

        // A pointer to the player game object
        wolf::GameObject* m_pPlayerObject = nullptr;

        // Manager for handling game states
        GameStateManager* m_pStateManager = nullptr;

        // Labyrinth builder component pointer
        LabyrinthBuilder* m_pLabyrinthBuilder = nullptr;

        // Flags
        bool m_showDebug = false;
        bool m_showLabyrinthBuilder = false;

        // Managers
        HitboxManager* m_pHitboxManager = nullptr;
        HurtboxManager* m_pHurtboxManager = nullptr;
};
