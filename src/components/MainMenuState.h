#pragma once
#include "GameState.h"
#include "Theseus.h"  

class MainMenuState : public GameState
{
public:
    // Pass both manager and gameInstance to the base class constructor
    MainMenuState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}  

    void Enter() override;
    void Exit() override;
    void Pause() override {}
    void Resume() override {}
    void Update(float delta) override;
    void Render() override;
};
