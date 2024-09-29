//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// Modifications : D'Anyil Landry
// ver 1.1
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once
#include "GameState.h"
#include <theseus.h> // Include the main game class
#include "W_Sprite2d.h"

#include "../HitboxManager.h"
#include "../HurtboxManager.h"
#include "EnemyController.h"

class LabyrinthManager;

class PlayState : public GameState
{
public:
    PlayState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}  

    void Enter() override;
    void Exit() override;
    void Pause() override;
    void Resume() override;
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender() override;

private:
    
    // Game objects / components that will exist for the duration of the play state
    wolf::GameObject* m_pPlayerObject = nullptr;
    LabyrinthManager* m_pLabyrinthManager = nullptr;
    wolf::GameObject* m_pMinitaurObject = nullptr;


    // Manager for hitboxes
    HitboxManager* m_pHitboxManager = nullptr;

    // Manager for hurtboxes
    HurtboxManager* m_pHurtboxManager = nullptr;

    // Flags
    bool m_showLabyrinthManager = false;

    // Private helper methods

    // Creates the player object and all of its components
    // PRE: The player must not have been created yet
    // POST: m_pPlayerObject will be set to a pointer to the newly created player object
    void CreatePlayer();
    void CreateMinitaurEnemy();
};
