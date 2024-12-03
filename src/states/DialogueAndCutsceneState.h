#pragma once

#include "GameState.h"
#include "Theseus.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>
#include "W_Texture.h"

class DialogueAndCutsceneState : public GameState {
public:
    DialogueAndCutsceneState(GameStateManager* manager, Theseus* gameInstance, const std::string& yamlFilePath);
    void Enter() override;
    void Exit() override;
    void Pause() override;
    void Resume() override;
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override {}
    void BackgroundRender() override {}
    void StartSequence(const std::string& sequenceID); // Start a sequence (dialogues and cutscenes)
    // New function to handle the lifecycle of sequence loading
    void LoadSequence(const std::string& sequenceID);

private:
    // Unified logic for sequences
    void AdvanceSequence(float delta); // Handles both dialogue and cutscene progression
    void RenderSequence();            // Handles rendering for both dialogue and cutscene

    // YAML Parsing
    void LoadFromYAML(const std::string& yamlFilePath);

    // Helper methods
    std::string GetCurrentDialogueLine() const;
    std::string GetCurrentCharacterName() const;
    void OnContinueButtonPressed();
    void EndDialogue();
    void OnExitButtonPressed();
    void ResetCutsceneState();

    // Sequence data structures
    struct CharacterData {
        std::string name;
        std::string portraitPath;
        wolf::Texture* portraitTexture = nullptr;
    };

    struct DialogueLine {
        std::string characterName;
        std::string text;
        float duration = 0.0f;
    };

    struct CameraKeyframe {
        glm::vec2 position;   // Position for the camera
        float zoom = 1.0f;    // Zoom level
        float duration = 0.0f; // Duration for this keyframe
        std::string target;   // Target entity (e.g., "Theseus", "Gorgon")
    };

    struct DialogueAndCutsceneItem {
        std::string sequenceID; // Identifier for the sequence
        std::string type; // "dialogue" or "cutscene"
        DialogueLine dialogue; // For dialogue items
        std::vector<CameraKeyframe> cutscene; // For cutscene items
        std::vector<CharacterData> characters; // Associated characters
    };

    // Sequence management
    std::vector<DialogueAndCutsceneItem> m_dialogueAndCutsceneSequence; // Unified sequence list
    size_t m_currentSequenceIndex = 0; // Current sequence item index

    // Dialogue state variables
    bool m_isDialogueActive = false;
    bool m_showFullText = false;
    bool m_isLineFinished = false;
    bool m_autoplay = false;
    float m_timeSinceLastKeyframe = 0.0f;
    float m_autoPlayDelay = 2.0f;
    size_t m_currentLineIndex = 0; // Tracks the current line in a dialogue

    // Cutscene state variables
    size_t m_currentKeyframeIndex = 0;
    float m_cutsceneTimer = 0.0f;
    glm::vec2 m_currentCameraPosition = glm::vec2(0.0f, 0.0f); // Current camera position
    float m_currentZoomLevel = 1.0f; // Current zoom level

    // YAML data
    std::string m_yamlFilePath;
    bool m_isYAMLLoaded = false;
    std::unordered_map<std::string, wolf::Texture*> m_characterPortraits; // Map for character portraits
    std::string m_currentCharacterName;

    float m_lmbCooldown = 0.0f; // Cooldown timer for LMB input
    const float LMB_DELAY = 0.75f; // Delay duration in seconds
};
