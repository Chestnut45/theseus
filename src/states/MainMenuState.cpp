//-----------------------------------------------------------------------------
// File:			MainMenuState.h
// Original Author:	Youssef Ashraf
// Modifications: D'Anyil Landry
// ver 1.1
// A class that's responsible for the Concrete Main Menu State.
//-----------------------------------------------------------------------------
#include "MainMenuState.h"
#include "PlayState.h"
#include <imgui/imgui.h>
#include <W_Audio.h>

void MainMenuState::Enter()
{
    // Initialize the main menu
    wolf::Audio::Stop();
    wolf::Audio::Play("data/sounds/bgm_title_screen.wav", 1.2f, 0.0f, 0.0f, false, true, 3.31f);
}

void MainMenuState::Exit()
{
    // Clean up the main menu
    wolf::Audio::Stop();
}

void MainMenuState::Update(float delta)
{
    const int w = m_pGameInstance->GetWidth();
    const int h = m_pGameInstance->GetHeight();
    const int buttonWidth = 160;
    const int buttonHeight = 48;

    // Set the UI window to cover the screen
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize({(float)w, (float)h});

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    // Make the window non-resizable, remove toolbar, etc.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoScrollWithMouse;

    // Set a background color for now until we make a splash screen
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.096f, 0.088f, 0.196f, 1.0f));

    // Begin the window
    ImGui::Begin("Main Menu", nullptr, flags);

    // Load the background image once
    static ImTextureID backgroundTextureID = nullptr;
    static wolf::Texture* pbackgroundTexture = nullptr;
    if (!pbackgroundTexture) {
        pbackgroundTexture = wolf::TextureManager::CreateTexture("data/textures/TheseusTitle.png");
        pbackgroundTexture->SetFilterMode(wolf::Texture::FilterMode::FM_Nearest, wolf::Texture::FilterMode::FM_Nearest);
        backgroundTextureID = reinterpret_cast<void*>(pbackgroundTexture->GetID());
    }

    // Darken background
    float shade = (m_screen == Screen::MAIN) ? 0.95f : (m_screen == Screen::FADE) ? 0.32f * (1.0f - m_fadeTime) : 0.32f;
    float buttonShade = (m_screen == Screen::MAIN) ? 1.0f : 0.64f;

    // Draw the background image
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::Image(backgroundTextureID, ImGui::GetWindowSize(), ImVec2(0,0), ImVec2(1,1), ImVec4(shade, shade, shade, 1.0));

    // Centered title
    ImVec2 dimensions = ImGui::GetWindowSize();

    // Spacing
    ImGui::NewLine();

    // Push the button style vars and colors
    // ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    // ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));
    // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f * buttonShade));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.75f * buttonShade));
    

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 32.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.75f * buttonShade));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.24f * buttonShade));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.75f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

    // Calculate some offsets
    float startX = (dimensions.x - buttonWidth) * 0.5f;
    float buttonY = dimensions.y * 0.74f;

    switch (m_screen)
    {
        case Screen::MAIN:

            ImGui::SetCursorPosX(startX - 10);
            ImGui::SetCursorPosY(buttonY - 64);
            if (ImGui::Button("Play", {buttonWidth + 20, buttonHeight}))
            {
                m_screen = Screen::SEED_SELECT;
            }
            ImGui::NewLine();
            ImGui::SetCursorPosX(startX + 16);
            if (ImGui::Button("Options", {buttonWidth - 32, buttonHeight - 8}))
            {
                m_screen = Screen::OPTIONS;
            }
            ImGui::NewLine();
            ImGui::SetCursorPosX(startX + 16);
            if (ImGui::Button("Quit", {buttonWidth - 32, buttonHeight - 8}))
            {
                m_pGameInstance->Shutdown();
            }
            
            if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) m_pGameInstance->Shutdown();

            break;
        
        case Screen::OPTIONS:
            
            // TODO: Serialize state to disk if we have time
            static bool fullscreen = false;
            static bool vsync = false;
            static float volume = 0.8f;
            static const float volumeOffset = 0.2f;
            
            ImGui::SetCursorPosX(startX - 47);
            ImGui::SetCursorPosY(buttonY - 65);
            ImGui::BeginChild("###Constraint", ImVec2(256, 20));
            ImGui::SeparatorText("Display");
            ImGui::EndChild();
            ImGui::SetCursorPosX(startX - 47);
            if (ImGui::Checkbox("Fullscreen", &fullscreen)) m_pGameInstance->SetFullscreen(fullscreen);
            ImGui::SetCursorPosX(startX - 47);
            if (ImGui::Checkbox("Vsync", &vsync)) m_pGameInstance->SetVsync(vsync);
            ImGui::SetCursorPosX(startX - 47);
            ImGui::BeginChild("###Constraint2", ImVec2(256, 20));
            ImGui::SeparatorText("Audio");
            ImGui::EndChild();
            ImGui::SetCursorPosX(startX - 47);
            ImGui::SetNextItemWidth(256);
            if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f"))
            {
                wolf::Audio::SetGlobalVolume(volume + volumeOffset);
            }

            ImGui::NewLine();
            ImGui::SetCursorPosX(startX + 16);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
            if (ImGui::Button("Back", {buttonWidth - 32, buttonHeight - 8}) || wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
            {
                m_screen = Screen::MAIN;
            }

            break;

        case Screen::SEED_SELECT:

            ImGui::SetCursorPosX(startX + 32);
            ImGui::SetCursorPosY(buttonY - 72);
            if (ImGui::Checkbox("Random Seed", &m_randomSeed))
            {
                if (m_randomSeed)
                {
                    m_seedText.clear();
                }
            }
            ImGui::SetCursorPosX(dimensions.x / 2 - 128);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
            ImGui::SetNextItemWidth(256);
            if (m_randomSeed) ImGui::BeginDisabled();
            ImGui::InputText("Seed", &m_seedText, ImGuiInputTextFlags_CharsNoBlank);
            if (m_randomSeed) ImGui::EndDisabled();
            ImGui::SetCursorPosX(startX - 10);
            ImGui::SetCursorPosY(buttonY);
            if (ImGui::Button("Enter the Labyrinth", {buttonWidth + 20, buttonHeight}))
            {
                wolf::Audio::Stop();
                wolf::Audio::Play("data/sounds/sfx_boss_growl.wav", 1.2f, -8000.0f);
                m_screen = Screen::FADE;
            }
            ImGui::NewLine();
            ImGui::SetCursorPosX(startX + 16);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
            if (ImGui::Button("Back", {buttonWidth - 32, buttonHeight - 8}) || wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE))
            {
                m_screen = Screen::MAIN;
                m_randomSeed = true;
                m_seedText.clear();
            }

            break;
        
        case Screen::FADE:
            
            if (m_fadeTime >= 1.0f)
            {
                m_pStateManager->PushState(new PlayState(m_pStateManager, m_pGameInstance, m_seedText));
            }
            m_fadeTime += delta;

            break;
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(11);

    // Close window and pop vars
    ImGui::End();
}

void MainMenuState::Render(float delta)
{
}

void MainMenuState::BackgroundUpdate(float delta)
{
}

void MainMenuState::BackgroundRender(float delta)
{
}