#include "DialogueState.h"
#include "W_Input.h"
#include <imgui/imgui.h>
#include "GameStateManager.h"
#include <iostream>

DialogueState::DialogueState(GameStateManager* manager, Theseus* gameInstance, DialogueManager* dialogueManager)
    : GameState(manager, gameInstance), m_pDialogueManager(dialogueManager)
{
    
}

void DialogueState::Enter()
{
    std::cout << "Entering Dialogue State." << std::endl;
    wolf::EventManager::AddListener<CutsceneTriggerEvent, DialogueState, &DialogueState::OnCutsceneTriggerEvent>(*this);
    wolf::EventManager::AddListener<DialogueResumeEvent, DialogueState, &DialogueState::OnDialogueResumeEvent>(*this);


    // Reset state variables on entering the dialogue state
    m_currentLineIndex = 0;
    m_isDialogueActive = true;  // Set active to true when entering
    m_timeSinceLastKeyframe = 0.0f;
    m_showFullText = false;
    m_shouldExit = false;  // Reset exit flag
}

void DialogueState::Exit()
{
    // Reset state variables and active dialogue flag on exit
    m_isDialogueActive = false;
    m_currentLineIndex = 0;
    wolf::EventManager::RemoveListener<DialogueResumeEvent, DialogueState, &DialogueState::OnDialogueResumeEvent>(*this);

}

void DialogueState::Update(float delta)
{
    if (m_shouldExit)  // Check if exit was requested
    {
        m_pStateManager->PopState();  // Perform the state exit
        return;
    }

    if (!m_isDialogueActive) return;  // No update if dialogue is not active

    // Update keyframe timing and determine if the current line is completely shown
    m_timeSinceLastKeyframe += delta;
    const std::string& currentLine = GetCurrentDialogueLine();
    m_isLineFinished = m_timeSinceLastKeyframe >= currentLine.length() * 0.05f || m_showFullText;

    // Handle user input or autoplay for dialogue progression
    bool isInputPressed = wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE);

    // Check if any UI elements like buttons are hovered/clicked
    bool isAnyButtonHovered = ImGui::IsAnyItemHovered();
    if (isInputPressed && !isAnyButtonHovered)
    {
        if (!m_showFullText && !m_isLineFinished)
        {
            // Show the full text if not already fully displayed
            m_showFullText = true;
        }
        else if (m_isLineFinished && m_currentLineIndex < m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID).size() - 1)
        {
            // If line is fully displayed, advance to the next line or handle end of dialogue
            AdvanceDialogue();
            m_timeSinceLastKeyframe = 0.0f;
            m_showFullText = false;  // Reset the flag for the next line
            m_isLineFinished = false;
        }
    }

    // Handle autoplay progression based on a timer, but stop at the last line
    if (m_autoplay && m_timeSinceLastKeyframe > m_autoPlayDelay && m_isLineFinished)
    {
        const auto& dialogueLines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);
        if (m_currentLineIndex < dialogueLines.size() - 1)
        {
            AdvanceDialogue();
            m_timeSinceLastKeyframe = 0.0f;
            m_showFullText = false;  // Reset the flag for the next line
            m_isLineFinished = false;
        }
    }
}

ImVec2 addImVec2(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}


void DialogueState::Render()
{
    if (!m_isDialogueActive) return;  // Skip rendering if dialogue is inactive

    // Smooth fade-in effect for the dialogue box
    static float fadeOpacity = 0.0f;
    fadeOpacity = std::min(fadeOpacity + 0.05f, 1.0f);  // Gradually increase opacity

    // Check if we're on the last line of the dialogue
    const auto& lines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);
    bool isLastLine = (m_currentLineIndex >= lines.size() - 1);

    // Screen dimensions
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float dialogueWidth = screenSize.x * 0.75f;  // Set width to 75% of screen
    float baseHeight = 100.0f;  // Base height for window padding and controls

    // Calculate text height
    const std::string& currentLine = GetCurrentDialogueLine();
    float textHeight = ImGui::CalcTextSize(currentLine.c_str(), nullptr, true, dialogueWidth - 100).y;

    // Add extra height if there's a portrait
    bool hasPortrait = false;
    float portraitHeight = 0.0f;
    wolf::Texture* portraitTexture = nullptr;

    if (!m_currentDialogueID.empty())
    {
        const std::string& characterName = GetCurrentCharacterName();
        for (auto& character : m_dialogueData[m_currentDialogueID].characters)
        {
            if (character.name == characterName)
            {
                portraitTexture = character.portraitTexture; // Retrieve the portrait texture
                hasPortrait = (portraitTexture != nullptr);
                portraitHeight = 64.0f;  // Set portrait height (adjust as needed)
                break;
            }
        }
    }

    // Calculate total window height
    float windowHeight = baseHeight + std::max(textHeight, portraitHeight) + 80.0f;  // Add padding for buttons and controls

    // Calculate window position
    ImVec2 windowSize = ImVec2(dialogueWidth, windowHeight);
    ImVec2 windowPos = ImVec2((screenSize.x - windowSize.x) / 2, screenSize.y - windowSize.y - 50);

    // Enhanced Layered Shadow Effect
    auto* drawList = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < 5; i++) {
        float shadowOpacity = 0.05f * (i + 1) * fadeOpacity;
        float shadowOffset = 10.0f + (i * 2.0f);
        drawList->AddRectFilled(
            addImVec2(windowPos, ImVec2(-shadowOffset, -shadowOffset)),
            addImVec2(addImVec2(windowPos, windowSize), ImVec2(shadowOffset, shadowOffset)),
            ImColor(0.0f, 0.0f, 0.0f, shadowOpacity),
            20.0f
        );
    }

    // Gradient Background
    drawList->AddRectFilledMultiColor(
        windowPos,
        addImVec2(windowPos, windowSize),
        ImColor(30, 34, 42, static_cast<int>(fadeOpacity * 255)),
        ImColor(30, 34, 42, static_cast<int>(fadeOpacity * 255)),
        ImColor(24, 26, 32, static_cast<int>(fadeOpacity * 255)),
        ImColor(24, 26, 32, static_cast<int>(fadeOpacity * 255))
    );

    // ImGui window styling
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 25));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 15));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.5f, 0.7f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.95f, fadeOpacity));

    ImGui::Begin("EnhancedDialogue", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Display character name and portrait if available
    if (!m_currentDialogueID.empty())
    {
        const std::string& characterName = GetCurrentCharacterName();
        if (hasPortrait && portraitTexture)
        {
            // Display the character portrait
            ImGui::Image((void*)(intptr_t)portraitTexture->GetID(), ImVec2(64, 64));
            ImGui::SameLine();
        }
        ImGui::TextColored(ImVec4(0.8f, 0.85f, 1.0f, 1.0f), "%s:", characterName.c_str());

        // Accent line above text
        ImVec2 start = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(0, -10));
        ImVec2 end = addImVec2(ImGui::GetCursorScreenPos(), ImVec2(windowSize.x * 0.25f, -10));
        drawList->AddLine(start, end, ImColor(0.5f, 0.7f, 1.0f, 0.6f), 2.0f);

        // Display dialogue text
        static float lineFade = 0.0f;
        lineFade = std::min(lineFade + 0.02f, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, lineFade * fadeOpacity);

        if (m_showFullText)
        {
            ImGui::TextWrapped("%s", currentLine.c_str());
        }
        else
        {
            std::string partialText = currentLine.substr(0, static_cast<int>(m_timeSinceLastKeyframe / 0.05f));
            partialText += "|";
            ImGui::TextWrapped("%s", partialText.c_str());
        }
        ImGui::PopStyleVar();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));  // Whitespace below text
    }

    // Centered button layout
    bool showContinueButton = !isLastLine || !m_isLineFinished;
    float buttonWidth = showContinueButton ? 130.0f : 180.0f;  // Increase width when only two buttons are shown
    ImVec2 buttonSize(buttonWidth, 35);

    float totalButtonWidth = showContinueButton ? 420.0f : 2 * buttonWidth;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2);  // Center buttons

    // Button styling for a polished look
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

    // Autoplay button
    if (ImGui::Button(m_autoplay ? "Autoplay: ON" : "Autoplay: OFF", buttonSize))
    {
        m_autoplay = !m_autoplay;
    }

    ImGui::PopStyleColor(3);
    ImGui::SameLine();

    // Continue button (if not last line or line is not finished)
    if (showContinueButton)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.45f, 0.5f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.55f, 0.6f, fadeOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));

        if (ImGui::Button("Continue", buttonSize))
        {
            OnContinueButtonPressed();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
    }

    // Exit button (always visible)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.35f, 0.4f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.4f, 0.45f, fadeOpacity));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.3f, 0.35f, fadeOpacity));

    if (ImGui::Button("Exit", buttonSize))
    {
        EndDialogue();
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}




void DialogueState::Pause()
{
}

void DialogueState::Resume()
{
}

void DialogueState::BackgroundUpdate(float delta)
{
    // Implement any background updates for the dialogue state (optional)
}

void DialogueState::BackgroundRender()
{
    // Implement any background rendering for the dialogue state (optional)
    m_pGameInstance->GetScene().Render();  // Render the current scene (including CutScene effects)

}

void DialogueState::StartDialogue(const std::string& dialogueID)
{
    // std::cout << "Starting dialogue with ID: " << dialogueID << std::endl;

    // Load the dialogue using the ID from the DialogueManager
    DialogueData* dialogue = m_pDialogueManager->GetDialogue(dialogueID);
    if (!dialogue) {
        m_isDialogueActive = false;
        return;
    }

    // Set the current dialogue and initialize state variables
    m_currentDialogueID = dialogueID;
    m_currentLineIndex = 0;
    m_isDialogueActive = true;
    m_showFullText = false;
    m_timeSinceLastKeyframe = 0.0f;

    // Populate the m_dialogueData with the dialogue from the DialogueManager
    m_dialogueData[m_currentDialogueID] = *dialogue;
}

void DialogueState::AdvanceDialogue() {
    const auto& dialogueLines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);

    if (m_currentLineIndex < dialogueLines.size() - 1) {
        m_currentLineIndex++;
        const auto& currentLine = dialogueLines[m_currentLineIndex];

        if (currentLine.action == "cutscene") {
            // Trigger the cutscene and temporarily exit dialogue
            m_shouldExit = true;
            wolf::EventManager::TriggerEvent(CutsceneTriggerEvent(currentLine.cutsceneID));
            return;
        }

        // Prepare for the next dialogue line
        m_showFullText = false;
        m_timeSinceLastKeyframe = 0.0f;

    } else {
        // Only set m_shouldExit when reaching the **final** dialogue line
        m_shouldExit = true;
    }
}

std::string DialogueState::GetCurrentDialogueLine() const
{
    // Retrieve the current line of dialogue based on the index
    const auto& dialogueLines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);
    if (m_currentLineIndex < dialogueLines.size())
    {
        return dialogueLines[m_currentLineIndex].text;
    }
    return "";  // Return an empty string if the index is out of bounds
}
std::string DialogueState::GetCurrentCharacterName() const
{
    // Check if the current dialogue ID is valid
    if (m_currentDialogueID.empty()) return "";

    // Get the dialogue associated with the current ID from the DialogueManager
    auto it = m_dialogueData.find(m_currentDialogueID);
    if (it == m_dialogueData.end()) {
        return "";
    }

    // Check if the current line index is valid
    const DialogueData& dialogue = it->second;
    if (m_currentLineIndex >= dialogue.lines.size()) {
        return "";
    }

    // Return the character name for the current line
    return dialogue.lines[m_currentLineIndex].characterName;
}

void DialogueState::OnContinueButtonPressed()
{
    // Get the current dialogue lines
    const auto& lines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);

    // Check if we are on the last line
    bool isLastLine = (m_currentLineIndex >= lines.size() - 1);

    if (!m_showFullText && !m_isLineFinished)
    {
        // If the text is not fully displayed, show the full text
        m_showFullText = true;
    }
    else if (!isLastLine)
    {
        // If it's not the last line, advance to the next line
        m_currentLineIndex++;
        m_showFullText = false;  // Reset to partial text display
        m_timeSinceLastKeyframe = 0.0f;  // Reset keyframe timer
    }
    else if (isLastLine && m_isLineFinished)
    {
        // If it's the last line and the text is fully displayed, end the dialogue
        EndDialogue();
    }
}
void DialogueState::OnSkipButtonPressed()
{
    if (!m_currentDialogueID.empty())
    {
        m_showFullText = true;  // Show the full text of the current line
    }
}

void DialogueState::EndDialogue()
{
    // Set dialogue state to inactive
    m_isDialogueActive = false;

    // Clear the current dialogue ID and reset the line index
    m_currentDialogueID = "";
    m_currentLineIndex = 0;
    m_showFullText = true;
    m_timeSinceLastKeyframe = 0.0f;

    // Set the flag to request state pop (return to previous state)
    m_shouldExit = true;

    std::cout << "Dialogue has ended. Transitioning back to PlayState." << std::endl;
}
void DialogueState::OnCutsceneTriggerEvent(const CutsceneTriggerEvent& event)
{
    // Ensure the event contains a valid cutsceneID
    if (!event.cutsceneID.empty())
    {
        std::cout << "Starting cutscene: " << event.cutsceneID << std::endl;

        // Push the CutSceneState with the provided cutsceneID
        m_pStateManager->PushState(new CutSceneState(m_pStateManager, m_pGameInstance, "data/cutscenes.yaml", event.cutsceneID));
    }
}

void DialogueState::OnDialogueResumeEvent(const DialogueResumeEvent& event) {
    if (!m_currentDialogueID.empty()) {
        wolf::Log("Resuming dialogue with ID: " + m_currentDialogueID + " after cutscene: " + event.m_cutsceneID);

        // Reset the exit flag
        m_shouldExit = false; // Prevent premature exit

        // Resume dialogue from where it left off
        AdvanceDialogue();
    } else {
        wolf::Log("No dialogue to resume after cutscene: " + event.m_cutsceneID);
    }
}