#pragma once
#include <imgui.h>
#include "GameState.h"
#include "DialogueManager.h"
#include "Theseus.h"
#include "CutSceneState.h"



// DialogueState class to manage displaying dialogue and transitions
class DialogueState : public GameState
{
public:
    // Constructor: Initialize with a manager, game instance, and dialogue manager
    DialogueState(GameStateManager* manager, Theseus* gameInstance, DialogueManager* dialogueManager);

    // Override lifecycle functions
    void Enter() override;
    void Exit() override;
    void Update(float delta) override;
    void Render() override;

    // Additional state management functions
    void Pause() override;
    void Resume() override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender() override;

    // Start a dialogue with a specific ID
    void StartDialogue(const std::string& dialogueID);
    void OnCutsceneTriggerEvent(const CutsceneTriggerEvent& event);
private:
    std::string GetCurrentCharacterName() const;
    void OnContinueButtonPressed();
    void OnSkipButtonPressed();
    void EndDialogue();
    
    // Helper function to progress to the next line in the dialogue
    void AdvanceDialogue();

    // Helper function to get the current line of dialogue text
    std::string GetCurrentDialogueLine() const;

    // DialogueManager reference to access dialogues
    DialogueManager* m_pDialogueManager = nullptr;
    

    // State variables for managing dialogue
    std::string m_currentDialogueID;          // The ID of the current dialogue
    size_t m_currentLineIndex = 0;             // The index of the current line in the dialogue
    bool m_isDialogueActive = false;           // Whether a dialogue is currently active
    bool m_autoplay = false;                   // Whether autoplay mode is enabled
    bool m_shouldExit = false;
    bool m_isLineFinished = false;

    // Keyframe and animation variables
    bool m_showFullText = false;               // Whether the full line text is displayed
    float m_timeSinceLastKeyframe = 0.0f;      // Time elapsed since the last keyframe
    const float m_autoPlayDelay = 2.0f;        // Delay between autoplay transitions

    // Replace 'Dialogue' with 'DialogueData'
    std::unordered_map<std::string, DialogueData> m_dialogueData;
};
