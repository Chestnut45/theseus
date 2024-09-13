//-----------------------------------------------------------------------------
// File:			GameStateManager.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible managing my game states.
//-----------------------------------------------------------------------------

#pragma once
#include "GameState.h"

class GameStateManager
{
public:
    GameStateManager() : m_currentState(nullptr) {}

    void SetState(GameState* newState)
    {
        if (m_currentState)
        {
            delete m_currentState;
        }
        m_currentState = newState;
    }

    void Update(float delta)
    {
        if (m_currentState)
        {
            m_currentState->Update(delta);
        }
    }

    void Render()
    {
        if (m_currentState)
        {
            m_currentState->Render();
        }
    }

private:
    GameState* m_currentState;
};
