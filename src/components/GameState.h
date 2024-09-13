//-----------------------------------------------------------------------------
// File:			GameState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Game States.
//-----------------------------------------------------------------------------
#include "wolf.h"
#pragma once

enum class GameStateType
{
    MAIN_MENU,
    OPTIONS,
    PLAY
};
class GameStateManager;
class GameState
{
public:
    GameState(GameStateManager* manager) : m_manager(manager) {}
    virtual ~GameState() = default;

    virtual void Update(float delta) = 0;
    virtual void Render() = 0;

protected:
    GameStateManager* m_manager;
};