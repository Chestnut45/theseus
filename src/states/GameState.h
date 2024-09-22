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
    GameState(GameStateManager* manager, Theseus* gameInstance)
        : m_pStateManager(manager), m_pGameInstance(gameInstance) {}
    virtual ~GameState() = default;

    // Lifecycle functions for states
    virtual void Enter() = 0;
    virtual void Exit() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;

    // Game loop functions
    // NOTE: Update and Render are only called when the
    // state is currently at the top of the stack.
    virtual void Update(float delta) = 0;
    virtual void Render() = 0;

    // Background game loop functions
    // NOTE: BackgroundUpdate and BackgroundRender are only
    // called when the state is not at the top of the stack.
    virtual void BackgroundUpdate(float delta) = 0;
    virtual void BackgroundRender() = 0;

protected:
    GameStateManager* m_pStateManager;   // State manager
    Theseus* m_pGameInstance;       // Reference to the game instance
};