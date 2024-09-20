#include "PauseState.h"
#include "MainMenuState.h"
#include "ImGui/imgui.h"

void PauseState::Enter()
{
    // Pause game logic, display pause menu
}

void PauseState::Exit()
{
    // Resume game logic
}

void PauseState::Update(float delta)
{
    ImGui::Begin("Paused");
    ImGui::Text("Game is paused.");
    
    if (ImGui::Button("Resume"))
    {
        m_manager->PopState();  // Pop the PauseState to resume the game
    }

    if (ImGui::Button("Main Menu"))
    {
        m_manager->ClearAndPushState(new MainMenuState(m_manager, m_gameInstance));  // Return to Main Menu
    }

    ImGui::End();
}

void PauseState::Render()
{
    // Render any visual effects of the pause state
}
