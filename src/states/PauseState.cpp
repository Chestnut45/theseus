//-----------------------------------------------------------------------------
// File:			PauseState.h
// Original Author:	Youssef Ashraf
// Modifications: Aurora Ryder
// ver 1.1
// A class responsible for the Concrete Pause State.
//-----------------------------------------------------------------------------
#include "PauseState.h"
#include "MainMenuState.h"
#include <imgui/imgui.h>
#include <W_Input.h>
#include <W_EventManager.h>
#include <events/PauseEvent.h>
#include <PlayerController.h>
#include <LabyrinthManager.h>

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
    bool mainmenu = false;

    // Escape key triggers resume next frame
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_ESCAPE)) resume = true;

    // Grab the seed
    int seed = 0;
    for (auto&&[_, manager] : m_pGameInstance->GetScene().Each<LabyrinthManager>())
    {
        seed = manager.GetSeed();
        break;
    }

    bool debugEnabled = false;
    for (auto&&[_, player] : m_pGameInstance->GetScene().Each<PlayerController>())
    {
        debugEnabled = player.m_debugHotkeys;
        break;
    }

    // Get the dimensions of the game window
    const int w = m_pGameInstance->GetWidth();
    const int h = m_pGameInstance->GetHeight();
    const int halfWidth = w / 2;
    const int halfHeight = h / 2;

    // Button sizes
    const int buttonWidth = 128;
    const int buttonHeight = 40;

    // Set the UI window to cover the game screen
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize({(float)w, (float)h});

    // Make the pause menu darken the entire screen
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

    // Make the window non-resizeable, remove toolbar, etc.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;

    // Begin the window
    ImGui::Begin("Paused", nullptr, flags);

    // Centered title
    ImVec2 dimensions = ImGui::GetWindowSize();

    // Figure out how much space we need to show all of the text
    float textWidth = ImGui::CalcTextSize("Paused").x * 2.0f + ImGui::CalcTextSize("Left-Click (held) - Charge Bow / Throw Object").x * 0.5f;
    
    // Position the Paused header based on that
    glm::vec2 pausedPos = glm::vec2((dimensions.x - textWidth) * 0.5f, (dimensions.y - (buttonHeight * 4)) * 0.5f);
    ImGui::SetCursorPosX(pausedPos.x);
    ImGui::SetCursorPosY(pausedPos.y);
    ImGui::TextColored(ImColor(128, 128, 128, 255), "Paused");

    // Spacing
    ImGui::NewLine();

    // Centered buttons

    // Push the button style vars and colors
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 32.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, 1.0f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3725f, 0.3725f, 0.3725f, 1.0f));

    // Position the Resume button based on where the Paused header is
    ImGui::SetCursorPosX(pausedPos.x - buttonWidth * 0.34f);
    static bool hovered0 = false;
    static bool wasHovered0 = false;
    if (ImGui::Button("Resume", {buttonWidth, buttonHeight}))
    {
        resume = true;
        wolf::Audio::Play("data/sounds/sfx_ui_select.wav", 0.15f);
    }
    hovered0 = ImGui::IsItemHovered();
    if (hovered0 && !wasHovered0)
    {
        wolf::Audio::Play("data/sounds/sfx_ui_hover.wav", 0.15f);
    }
    wasHovered0 = hovered0;

    // Single character for spacing
    ImGui::Text(" ");

    // Position the Main Menu button based on where the Paused header is
    ImGui::SetCursorPosX(pausedPos.x - buttonWidth * 0.34f);
    static bool hovered1 = false;
    static bool wasHovered1 = false;
    if (ImGui::Button("Main Menu", {buttonWidth, buttonHeight}))
    {
        mainmenu = true;
        wolf::Audio::Play("data/sounds/sfx_ui_select.wav", 0.15f);
    }
    hovered1 = ImGui::IsItemHovered();
    if (hovered1 && !wasHovered1)
    {
        wolf::Audio::Play("data/sounds/sfx_ui_hover.wav", 0.15f);
    }
    wasHovered1 = hovered1;

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);

    // !-------------------- Aurora added this ----------------------!

    // Newline for style
    ImGui::NewLine();

    // Figure out where the Controls header and text will be based on the position of the Paused header
    float controlsPosX = pausedPos.x + ImGui::CalcTextSize("Left-Click (held) - Charge Bow / Throw Object").x * 0.5f;

    // Show the controls header
    ImGui::SetCursorPosX(controlsPosX);
    ImGui::SetCursorPosY(pausedPos.y);
    ImGui::TextColored(ImColor(128, 128, 128, 255), "Controls");

    ImGui::NewLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // List each of the controls and what they do

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("WASD - Move");

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("Spacebar - Roll");

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("E - Interact / Pickup");

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("M - Open / Close Minimap");

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("Left Alt - Quick Inventory");

    ImGui::SetCursorPosX(controlsPosX);
    ImGui::Text("Left-Click - Attack / Select / Charge / Throw");

    if (m_pGameInstance->IsDebugMode())
    {
        float dbgPosX = 12;
        float dbgPosY = 12;
        ImGui::NewLine();
        ImGui::NewLine();
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::SetCursorPosY(dbgPosY);
        ImGui::TextColored(ImColor(128, 128, 128, 255), "Debug Controls");
        ImGui::NewLine();
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("` - Performance Window");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("; - Daedalus' Terminal");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("/ - Noclip");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("\\ - Collider Debug View");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("- - Zoom out");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("+ - Zoom in");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("1 - Fill Inventory");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("2 - Give Gold");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("3 - Take Gold");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("7 - Nav Mesh Debug View");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Up - Apply Regeneration");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Down - Apply Petrification");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Left - Apply Burning");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Right - Apply Poison");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Home - Teleport to Spawn");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Right Shift - Teleport to Minotaur's Chamber");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Page Up - Superspeed");
        ImGui::SetCursorPosX(dbgPosX);
        ImGui::Text("Page Down - Godmode");
    }

    // Pop the style color we added
    ImGui::PopStyleColor(1);

    // !-------------------------------------------------------------!

    // Debug information
    ImGui::SetCursorPosX(12);
    ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 24);
    std::string debugString = debugEnabled ? "- Debug Mode " : "";
    ImGui::Text("Theseus v1.2 %s- Seed: %d", debugString.data(), seed);

    // Close window and pop vars
    ImGui::End();

    // Resume if flag is set
    if (resume)
    {
        wolf::EventManager::TriggerEvent(PauseEvent(false));
        m_pStateManager->PopState();
    }

    if (mainmenu)
    {
        m_pStateManager->ClearAndPushState(new MainMenuState(m_pStateManager, m_pGameInstance));
    }
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