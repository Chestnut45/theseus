#include "PauseState.h"
#include "MainMenuState.h"
#include <imgui/imgui.h>
#include <W_Input.h>

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
    // Unpause with escape key
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) m_pStateManager->PopState();

    // Display the pause menu
    ImGui::Begin("Paused");
    ImGui::Text("Game is paused.");
    
    if (ImGui::Button("Resume"))
    {
        m_pStateManager->PopState();  // Pop the PauseState to resume the game
    }

    if (ImGui::Button("Main Menu"))
    {
        m_pStateManager->ClearAndPushState(new MainMenuState(m_pStateManager, m_pGameInstance));  // Return to Main Menu
    }

    ImGui::End();
}

void PauseState::Render()
{
    // Render any visual effects of the pause state
}
