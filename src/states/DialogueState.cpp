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

    // Reset state variables on entering the dialogue state
    m_currentLineIndex = 0;
    m_isDialogueActive = true;  // Set active to true when entering
    m_timeSinceLastKeyframe = 0.0f;
    m_showFullText = false;
    m_shouldExit = false;  // Reset exit flag
}

void DialogueState::Exit()
{
    std::cout << "Exiting Dialogue State." << std::endl;

    // Reset state variables and active dialogue flag on exit
    m_isDialogueActive = false;
    m_currentLineIndex = 0;
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


void DialogueState::Render()
{
    if (!m_isDialogueActive) return;  // No rendering if dialogue is not active

    // Get the screen dimensions to position the dialogue box
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImVec2 windowSize = ImVec2(screenSize.x * 0.7f, screenSize.y * 0.3f); // 70% width and 30% height of the screen
    ImVec2 windowPos = ImVec2((screenSize.x - windowSize.x) / 2, screenSize.y - windowSize.y - 50); // Centered horizontally, slightly above the bottom

    // Set up style variables for a more visually appealing dialogue box
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);  // Rounded corners
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20)); // Padding around the content
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));   // Spacing between items

    // Colors
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));  // Slightly transparent dark background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));    // Gold border color for a premium look
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));   // Light text color

    // Start the ImGui window
    ImGui::Begin("Dialogue", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Display the current dialogue line
    if (!m_currentDialogueID.empty())
    {
        const std::string& currentLine = GetCurrentDialogueLine();

        // Display character name with color styling
        const std::string& characterName = GetCurrentCharacterName();
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f), "%s:", characterName.c_str());

        // Display the dialogue text with a typewriter effect or full text
        ImGui::Separator(); // Separator between name and text
        if (m_showFullText)
        {
            ImGui::TextWrapped("%s", currentLine.c_str());
        }
        else
        {
            std::string partialText = currentLine.substr(0, static_cast<int>(m_timeSinceLastKeyframe / 0.05f));
            ImGui::TextWrapped("%s", partialText.c_str());
        }

        // Add padding below the text for separation
        ImGui::Dummy(ImVec2(0.0f, 20.0f));  // Dummy widget for spacing
    }

    // Draw the autoplay toggle button centered below the dialogue text
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 150) / 2);
    if (ImGui::Button(m_autoplay ? "Autoplay: ON" : "Autoplay: OFF", ImVec2(150, 30)))
    {
        m_autoplay = !m_autoplay;
    }

    // If on the last dialogue line and fully displayed, show the Continue button to exit the dialogue
    const auto& lines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);
    if (m_currentLineIndex >= lines.size() - 1 && m_isLineFinished)
    {
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 150) / 2);
        if (ImGui::Button("Continue", ImVec2(150, 30)))
        {
            OnContinueButtonPressed();  // Trigger dialogue end
        }
    }

    // End the ImGui window and revert style variables/colors
    ImGui::End();
    ImGui::PopStyleVar(3);  // Revert style variables we pushed
    ImGui::PopStyleColor(3);  // Revert style colors we pushed
}

void DialogueState::Pause()
{
    std::cout << "Dialogue State Paused." << std::endl;
}

void DialogueState::Resume()
{
    std::cout << "Dialogue State Resumed." << std::endl;
}

void DialogueState::BackgroundUpdate(float delta)
{
    // Implement any background updates for the dialogue state (optional)
}

void DialogueState::BackgroundRender()
{
    // Implement any background rendering for the dialogue state (optional)
}

void DialogueState::StartDialogue(const std::string& dialogueID)
{
    std::cout << "Starting dialogue with ID: " << dialogueID << std::endl;

    // Load the dialogue using the ID from the DialogueManager
    DialogueData* dialogue = m_pDialogueManager->GetDialogue(dialogueID);
    if (!dialogue) {
        std::cerr << "Dialogue with ID " << dialogueID << " not found!" << std::endl;
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

void DialogueState::AdvanceDialogue()
{
    if (!m_isDialogueActive)
    {
        std::cout << "DialogueState::AdvanceDialogue() called when dialogue is inactive or exiting. Ignoring." << std::endl;
        return;
    }

    // Get the list of dialogue lines from the manager
    const auto& dialogueLines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);

    if (m_currentLineIndex < dialogueLines.size() - 1)
    {
        // Move to the next line in the dialogue
        m_currentLineIndex++;
        std::cout << "Advancing to next line. Current line index: " << m_currentLineIndex << std::endl;
        m_showFullText = false;  // Reset the display for the next line
    }
    else
    {
        // If no more lines are left, set exit flag and request state pop
        std::cout << "Reached the end of dialogue. Exiting dialogue state." << std::endl;
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
        std::cerr << "Dialogue ID " << m_currentDialogueID << " not found in m_dialogueData!" << std::endl;
        return "";
    }

    // Check if the current line index is valid
    const DialogueData& dialogue = it->second;
    if (m_currentLineIndex >= dialogue.lines.size()) {
        std::cerr << "Line index " << m_currentLineIndex << " out of bounds for dialogue ID " << m_currentDialogueID << std::endl;
        return "";
    }

    // Return the character name for the current line
    return dialogue.lines[m_currentLineIndex].characterName;
}

void DialogueState::OnContinueButtonPressed()
{
    if (m_currentDialogueID.empty()) return;

    // Get the current dialogue lines
    const auto& lines = m_pDialogueManager->GetDialogueLinesById(m_currentDialogueID);

    // Check if there are more lines to show
    if (m_currentLineIndex < lines.size() - 1)
    {
        // Move to the next line
        m_currentLineIndex++;
        m_showFullText = false;  // Reset to partial text display
        m_timeSinceLastKeyframe = 0.0f;  // Reset keyframe timer
    }
    else
    {
        // If no more lines, end the dialogue
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