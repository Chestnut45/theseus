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
    // Shutdown with escape key on main menu too
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) m_pGameInstance->Shutdown();

    // Get the dimensions of the game window
    const int w = m_pGameInstance->GetWidth();
    const int h = m_pGameInstance->GetHeight();
    const int halfWidth = w / 2;
    const int halfHeight = h / 2;

    // Menu window size
    const int menuWidth = 256;
    const int menuHeight = 256;

    // Button sizes
    const int buttonWidth = 128;
    const int buttonHeight = 32;

    // Set the UI window to cover the screen
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize({(float)w, (float)h});

    // Make the window non-resizeable, remove toolbar, etc.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    // Set a background color for now until we make a splash screen
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.096f, 0.088f, 0.196f, 1.0f));

    // Begin the window
    ImGui::Begin("Main Menu", nullptr, flags);

    // Centered title
    ImVec2 dimensions = ImGui::GetWindowSize();
    float textWidth = ImGui::CalcTextSize("Theseus").x;
    ImGui::SetCursorPosX((dimensions.x - textWidth) * 0.5f);
    ImGui::SetCursorPosY((dimensions.y - (buttonHeight * 6)) * 0.5f);
    ImGui::TextColored(ImColor(255, 215, 0, 255), "Theseus");

    // Spacing
    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();

    // Centered buttons

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Play", {buttonWidth, buttonHeight}))
    {
        m_pStateManager->PushState(new PlayState(m_pStateManager, m_pGameInstance));
    }

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Options", {buttonWidth, buttonHeight}))
    {
        // TODO: Options menu (when necessary)
    }

    ImGui::SetCursorPosX((dimensions.x - buttonWidth) * 0.5f);
    if (ImGui::Button("Quit", {buttonWidth, buttonHeight}))
    {
        m_pGameInstance->Shutdown();
    }

    // Close window and pop vars
    ImGui::End();
    ImGui::PopStyleColor();
}

void MainMenuState::Render()
{
}

void MainMenuState::BackgroundUpdate(float delta)
{
}

void MainMenuState::BackgroundRender()
{
}