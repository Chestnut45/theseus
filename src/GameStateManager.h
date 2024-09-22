//-----------------------------------------------------------------------------
// File:			GameStateManager.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class that's responsible for the State Manager
// forward declaration for the GameState.
//-----------------------------------------------------------------------------
#pragma once
#include <stack>

class GameState;

class GameStateManager
{
public:
    GameStateManager() = default;
    ~GameStateManager() { Clear(); }

    // push a new state onto the stack
    void PushState(GameState* state);
    // pop the current state off the stack
    void PopState();
    // clear all states and push a new one
    void ClearAndPushState(GameState* state);
    // get the current active state
    GameState* GetActiveState() const;
    // update the active state
    void Update(float delta);
    // render the active state
    void Render();
    // clear the stack and remove all states
    void Clear();

private:
    std::stack<GameState*> m_stateStack;  // Stack 
};