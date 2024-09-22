#include "MainMenuState.h"
#include "PlayState.h"
#include <imgui/imgui.h>

void MainMenuState::Enter()
{
    // Initialize the main menu
}

void MainMenuState::Exit()
{
    // Clean up the main menu
}

void MainMenuState::Update(float delta)
{
    // Render the main menu UI
    ImGui::Begin("Main Menu");
    ImGui::Text("Theseus - Main Menu");

    if (ImGui::Button("Play"))
    {
        m_pStateManager->PushState(new PlayState(m_pStateManager, m_pGameInstance));
    }

    if (ImGui::Button("Quit"))
    {
        m_pGameInstance->Shutdown();  // Properly shut down the game
    }

    ImGui::End();
}

void MainMenuState::Render()
{
    // Render the main menu visuals here if necessary
}