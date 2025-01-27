#include "MainMenuState.h"
#include "PlayState.h"
#include <imgui/imgui.h>
#include <W_Audio.h>

void MainMenuState::Enter()
{
    // Initialize the main menu
    wolf::Audio::Stop();
    wolf::Audio::Play("data/sounds/bgm_title_screen.wav", 0.8f, 0.0f, 0.0f, true, 3.31f);
}

void MainMenuState::Exit()
{
    // Clean up the main menu
    wolf::Audio::Stop();
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
    const int buttonHeight = 48;

    // Set the UI window to cover the screen
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize({(float)w, (float)h});

    // Make the window non-resizeable, remove toolbar, etc.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    // Set a background color for now until we make a splash screen
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.096f, 0.088f, 0.196f, 1.0f));

    // Begin the window
    ImGui::Begin("Main Menu", nullptr, flags);

    // Load the background image once
    static ImTextureID backgroundTextureID = nullptr;
    static wolf::Texture* pbackgroundTexture = nullptr;
    if (!pbackgroundTexture) {
        pbackgroundTexture = wolf::TextureManager::CreateTexture("data/textures/TheseusTitle.png");
        backgroundTextureID = reinterpret_cast<void*>(pbackgroundTexture->GetID());
    }

    // Draw the background image
    ImGui::Image(backgroundTextureID, ImGui::GetWindowSize(), ImVec2(0,0), ImVec2(1,1));

    // Centered title
    ImVec2 dimensions = ImGui::GetWindowSize();

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
    ImGui::SetCursorPosY(dimensions.y * 0.75f);
    if (ImGui::Button("Play", {buttonWidth, buttonHeight}))
    {
        m_pStateManager->PushState(new PlayState(m_pStateManager, m_pGameInstance, &m_pGameInstance->GetDialogueManager()));
    }

    ImGui::SetCursorPosX((dimensions.x - 4*buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(dimensions.y * 0.75f);
    if (ImGui::Button("Options", {buttonWidth, buttonHeight}))
    {
        // TODO: Options menu (when necessary)
    }

    ImGui::SetCursorPosX((dimensions.x + 2*buttonWidth) * 0.5f);
    ImGui::SetCursorPosY(dimensions.y * 0.75f);
    if (ImGui::Button("Quit", {buttonWidth, buttonHeight}))
    {
        m_pGameInstance->Shutdown();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);

    // Close window and pop vars
    ImGui::End();
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