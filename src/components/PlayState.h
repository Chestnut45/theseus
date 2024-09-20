//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// ver 1.0
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once
#include "GameState.h"
#include "Theseus.h"  // Include the main game class


class LabyrinthBuilder;

class PlayState : public GameState
{
public:
    PlayState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}  // Correctly pass both arguments

    void Enter() override;
    void Exit() override;
    void Pause() override;
    void Resume() override;
    void Update(float delta) override;
    void Render() override;

private:
    bool m_isPaused = false;
    wolf::GameObject* m_pPlayerObject = nullptr;  // Pointer to the player object
    LabyrinthBuilder* m_pLabyrinthBuilder = nullptr; // Labyrinth builder
};
