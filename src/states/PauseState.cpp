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

    // Get the dimensions of the game window
    const int w = m_pGameInstance->GetWidth();
    const int h = m_pGameInstance->GetHeight();
    const int halfWidth = w / 2;
    const int halfHeight = h / 2;

    // Button sizes
    const int buttonWidth = 128;
    const int buttonHeight = 32;

    // Set the UI window to cover the game screen
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize({(float)w, (float)h});

    // Make the pause menu darken the entire screen
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));

    // Make the window non-resizeable, remove toolbar, etc.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    // Begin the window
    ImGui::Begin("Paused", nullptr, flags);

    // Centered title
    ImVec2 dimensions = ImGui::GetWindowSize();
    float textWidth = ImGui::CalcTextSize("Paused").x;
    ImGui::SetCursorPosX((dimensions.x - textWidth) * 0.5f);
    ImGui::SetCursorPosY((dimensions.y - (buttonHeight * 4)) * 0.5f);
    ImGui::TextColored(ImColor(128, 128, 128, 255), "Paused");

    // Spacing
    ImGui::NewLine();

    // Centered buttons

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Resume", {buttonWidth, buttonHeight}))
    {
        m_pStateManager->PopState();
    }

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Main Manu", {buttonWidth, buttonHeight}))
    {
        m_pStateManager->ClearAndPushState(new MainMenuState(m_pStateManager, m_pGameInstance));
    }

    // Close window and pop vars
    ImGui::End();
    ImGui::PopStyleColor();
}

void PauseState::Render()
{
    // Pause state render logic
}

void PauseState::BackgroundUpdate(float delta)
{
    // Update logic for when state is inactive
}

void PauseState::BackgroundRender()
{
    // Render logic for when state is inactive
}