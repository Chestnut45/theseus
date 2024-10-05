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

    // Update keyframe timing
    m_timeSinceLastKeyframe += delta;

    // Handle user input or autoplay for dialogue progression
    if (wolf::Input::IsLMBJustDown() || wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE))
    {
        // Skip keyframe animation and show the full text, or move to the next line
        if (m_showFullText)
        {
            AdvanceDialogue();
        }
        else
        {
            m_showFullText = true;
        }
        m_timeSinceLastKeyframe = 0.0f;
    }

    // Handle autoplay progression based on a timer
    if (m_autoplay && m_timeSinceLastKeyframe > m_autoPlayDelay)
    {
        AdvanceDialogue();
        m_timeSinceLastKeyframe = 0.0f;
    }
}


void DialogueState::Render()
{
    if (!m_isDialogueActive) return;  // No rendering if dialogue is not active

    ImGui::Begin("Dialogue");

    if (!m_currentDialogueID.empty())
    {
        // Get the current line of dialogue based on the index
        const std::string& currentLine = GetCurrentDialogueLine();

        // Display the dialogue text with the keyframe animation or full text
        ImGui::TextWrapped("%s", m_showFullText ? currentLine.c_str() : GetCurrentDialogueLine().substr(0, static_cast<int>(m_timeSinceLastKeyframe / 0.05f)).c_str());
    }

    // Render autoplay button for user to toggle
    if (ImGui::Button("Autoplay"))
    {
        m_autoplay = !m_autoplay;
    }

    ImGui::End();
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

    // Load the dialogue using the ID and initialize state variables
    m_currentDialogueID = dialogueID;
    m_currentLineIndex = 0;
    m_isDialogueActive = true;
    m_showFullText = false;
    m_timeSinceLastKeyframe = 0.0f;
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

