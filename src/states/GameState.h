//-----------------------------------------------------------------------------
// File:			GameState.h
// Original Author:	Youssef Ashraf
// ver 1.2
// A class that's responsible for the Game States.
// w/forward declaration.
//-----------------------------------------------------------------------------
#pragma once
// Forward declare GameStateManager and Theseus to avoid circular dependencies
class GameStateManager;
class Theseus;

class GameState
{
public:
    // Constructor and destructor
    GameState(GameStateManager* manager, Theseus* gameInstance)
        : m_pStateManager(manager), m_pGameInstance(gameInstance) {}
    virtual ~GameState() {}

    // Pure virtual functions that derived classes must implement
    virtual void Enter() = 0;
    virtual void Exit() = 0;
    virtual void Update(float delta) = 0;
    virtual void Render(float delta) = 0;

    // Additional pure virtual functions for handling state lifecycle
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void BackgroundUpdate(float delta) = 0;
    virtual void BackgroundRender(float delta) = 0;

protected:
    GameStateManager* m_pStateManager = nullptr;  // Pointer to the state manager
    Theseus* m_pGameInstance = nullptr;           // Pointer to the main game instance
};