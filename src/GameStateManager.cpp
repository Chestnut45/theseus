//-----------------------------------------------------------------------------
// File:			GameStateManager.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class that's responsible for the State Manager
// forward declaration for the GameState.
//-----------------------------------------------------------------------------
#include "GameStateManager.h"
#include <states/GameState.h>
#include <stack>
 
void GameStateManager::PushState(GameState* state)
{
    if (!m_stateStack.empty())
    {
        m_stateStack.back()->Pause();
    }
    m_stateStack.push_back(state);
    state->Enter();
}

void GameStateManager::PopState()
{
    if (!m_stateStack.empty())
    {    
        // Exit and delete the top state
        m_stateStack.back()->Exit();
        delete m_stateStack.back();
        m_stateStack.pop_back();

        // Resume the next state if it exists
        if (!m_stateStack.empty())
        {
            m_stateStack.back()->Resume();
        }
    }
}

void GameStateManager::ClearAndPushState(GameState* state)
{
    Clear();
    PushState(state);
}

GameState* GameStateManager::GetActiveState() const
{
    return m_stateStack.empty() ? nullptr : m_stateStack.back();
}

void GameStateManager::Update(float delta)
{
    // Background update all the lower states in the stack
    for (int i = 0; i < m_stateStack.size() - 1; ++i)
    {
        m_stateStack[i]->BackgroundUpdate(delta);
    }

    // Update the main active state at the top of the stack
    if (!m_stateStack.empty()) m_stateStack.back()->Update(delta);
}

void GameStateManager::Render(float delta)
{
    // Background render all the lower states in the stack
    for (int i = 0; i < m_stateStack.size() - 1; ++i)
    {
        m_stateStack[i]->BackgroundRender(delta);
    }

    // Render the main active state at the top of the stack
    if (!m_stateStack.empty()) m_stateStack.back()->Render(delta);
}

void GameStateManager::Clear()
{
    while (!m_stateStack.empty())
    {
        // Exit and delete all states, top to bottom
        m_stateStack.back()->Exit();
        delete m_stateStack.back();
        m_stateStack.pop_back();
    }
}
