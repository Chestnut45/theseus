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
    void StartDialogue(const std::string& dialogueID);


private:
    // Dialogue logic
    void AdvanceDialogue();
    void EndDialogue();
    bool IsComplete() const;
    void OnContinueButtonPressed();
    std::string GetCurrentDialogueLine() const;
    std::string GetCurrentCharacterName() const;
    void UpdateDialogue(float delta);
    void RenderDialogue();

    // Cutscene logic
    void AdvanceCutscene(float delta);
    void StartCutscene(const std::string& cutsceneID);
    void EndCutscene();

    // YAML Parsing
    void LoadFromYAML(const std::string& yamlFilePath);


    // State management
    enum class Mode { Dialogue, Cutscene, None };
    Mode m_mode = Mode::None;

    // Dialogue data
    struct CharacterData {
        std::string name;
        std::string portraitPath;
        wolf::Texture* portraitTexture = nullptr;
    };

    struct DialogueLine {
        std::string characterName;
        std::string text;
        std::string action;
        std::string cutsceneID;
        float duration = 0.0f;
    };

    struct DialogueData {
        std::vector<CharacterData> characters;
        std::vector<DialogueLine> lines;
    };

    std::unordered_map<std::string, DialogueData> m_dialogues;
    std::vector<DialogueLine> m_currentDialogueLines;
    std::string m_activeDialogueID;
    size_t m_currentLineIndex = 0;

    // Cutscene data
    struct CameraKeyframe {
        glm::vec2 position;   // Position for the camera
        float zoom;           // Zoom level
        float duration;       // Duration for this keyframe
        std::string target;   // Target entity (e.g., "Theseus", "Gorgon")
    };

    std::unordered_map<std::string, std::vector<CameraKeyframe>> m_cutscenes;
    std::vector<CameraKeyframe> m_currentCutsceneKeyframes;
    size_t m_currentKeyframeIndex = 0;
    float m_cutsceneTimer = 0.0f;

    // Dialogue state variables
    bool m_isDialogueActive = false;
    bool m_showFullText = false;
    bool m_isLineFinished = false;
    bool m_autoplay = false;
    float m_timeSinceLastKeyframe = 0.0f;
    float m_autoPlayDelay = 2.0f;

    glm::vec2 m_currentCameraPosition = glm::vec2(0.0f, 0.0f); // Current camera position
    float m_currentZoomLevel = 1.0f; // Current zoom level

    // YAML data
    std::string m_yamlFilePath;
};
