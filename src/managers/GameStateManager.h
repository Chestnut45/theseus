//-----------------------------------------------------------------------------
// File:			GameStateManager.h
// Original Author:	Youssef Ashraf
// ver 1.1
// A class that's responsible for the State Manager
// forward declaration for the GameState.
//-----------------------------------------------------------------------------
#pragma once
#include <vector>

class GameState;

class GameStateManager
{
public:
    GameStateManager() = default;
    ~GameStateManager() { Clear(); }

    // Push a new state onto the stack
    void PushState(GameState* state);

    // Pop the current state off the stack
    void PopState();

    // Clear all states and push a new one
    void ClearAndPushState(GameState* state);

    // Get the current active state
    GameState* GetActiveState() const;

    // Update the active state and background update the lower states
    void Update(float delta);

    // Render the active state and background render the lower states
    void Render(float delta);

    // Clear the stack and remove all states
    void Clear();

private:
    
    // Stack of game states
    // NOTE: Implemented as a vector, so we can background update
    // and render all the states below the current top of the stack.
    std::vector<GameState*> m_stateStack;
};