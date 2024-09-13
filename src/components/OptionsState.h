//-----------------------------------------------------------------------------
// File:			OptionState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Concrete Option State.
//-----------------------------------------------------------------------------

#pragma once
#include "GameState.h"
#include "GameStateManager.h"  // Include the full definition of GameStateManager

class OptionsState : public GameState
{
public:
    OptionsState(GameStateManager* pManager) : GameState(pManager) {}

    void Update(float delta) override
    {
        // Implement Options statec logic
        // Example: Navigate through options menu
    }

    void Render() override
    {
        // Implement Option state rendering
        // Example: Render options menu UI
    }
};
