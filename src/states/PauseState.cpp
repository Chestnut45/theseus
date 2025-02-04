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
    // Resume flag (deferred until end of function for safety)
    bool resume = false;

    // Escape key triggers resume next frame
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) resume = true;

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

    // Push the button style vars and colors
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Resume", {buttonWidth, buttonHeight}))
    {
        resume = true;
    }

    // Single character for spacing
    ImGui::Text(" ");

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Main Menu", {buttonWidth, buttonHeight}))
    {
        m_pStateManager->ClearAndPushState(new MainMenuState(m_pStateManager, m_pGameInstance));
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);

    // Close window and pop vars
    ImGui::End();

    // Resume if flag is set
    if (resume) m_pStateManager->PopState();
}

void PauseState::Render(float delta)
{
    // Pause state render logic
}

void PauseState::BackgroundUpdate(float delta)
{
    // Update logic for when state is inactive
}

void PauseState::BackgroundRender(float delta)
{
    // Render logic for when state is inactive
}