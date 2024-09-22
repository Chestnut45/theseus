//-----------------------------------------------------------------------------
// File:			MainMenuState.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class that's responsible for the Concrete Main Menu State.
//-----------------------------------------------------------------------------
#pragma once
#include "GameState.h"
#include <theseus.h>

class MainMenuState : public GameState
{
public:
    // Pass both manager and gameInstance to the base class constructor
    MainMenuState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}  

    void Enter() override;
    void Exit() override;
    void Pause() override {}
    void Resume() override {}
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender() override;
};
