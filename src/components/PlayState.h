//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------

#pragma once
#include "GameState.h"
#include "GameStateManager.h" 

class PlayState : public GameState
{
public:
    PlayState(GameStateManager* manager) : GameState(manager) {}

    void Update(float delta) override
    {
        // Implement PlayState-specific logic
        // Example: Game logic update (player movement, enemies, etc.)
    }

    void Render() override
    {
        // Implement PlayState-specific rendering
        // Example: Render game objects, HUD, etc.
    }
};