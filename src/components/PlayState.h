//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once
#include "GameState.h"
#include "theseus.h"  // Include the main game class


class LabyrinthBuilder;

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

private:
    
    // Game objects / components that will exist for the duration of the play state
    wolf::GameObject* m_pPlayerObject = nullptr;
    LabyrinthBuilder* m_pLabyrinthBuilder = nullptr;

    // Flags
    bool m_isPaused = false;
    bool m_showLabyrinthBuilder = false;
};
