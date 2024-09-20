#include "MainMenuState.h"
#include "PlayState.h"
#include "ImGui/imgui.h"

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
        m_manager->PushState(new PlayState(m_manager, m_gameInstance));
    }

    if (ImGui::Button("Quit"))
    {
        m_gameInstance->Shutdown();  // Properly shut down the game
    }

    ImGui::End();
}

void MainMenuState::Render()
{
    // Render the main menu visuals here if necessary
}