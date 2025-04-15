#pragma once

//-----------------------------------------------------------------------------
// File:			MainMenuState.h
// Original Author:	Youssef Ashraf
// Modifications: D'Anyil Landry
// ver 1.1
// A class that's responsible for the Concrete Main Menu State.
//-----------------------------------------------------------------------------
#include "GameState.h"
#include <Theseus.h>

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
    void Render(float delta) override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender(float delta) override;

private:

    enum class Screen
    {
        MAIN,
        OPTIONS,
        SEED_SELECT,
        FADE
    };

    Screen m_screen = Screen::MAIN;
    std::string m_seedText;
    bool m_debugModeEnabled = false;
    bool m_randomSeed = true;
    float m_fadeTime = 0.0f;
};
