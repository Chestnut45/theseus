//-----------------------------------------------------------------------------
// File:			PauseState.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class responsible for the Concrete Pause State.
//-----------------------------------------------------------------------------

#pragma once
#include "GameState.h"
#include <theseus.h> // Include the main game class

class PauseState : public GameState
{
public:
    // Pass both manager and gameInstance to the base class constructor
    PauseState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}  // Fix: pass both arguments

    void Enter() override;
    void Exit() override;
    void Pause() override {}
    void Resume() override {}
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender() override;
};
