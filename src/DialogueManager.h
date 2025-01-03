#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "W_Texture.h"
#include "W_TextureManager.h"

// Structure to hold character-specific data
struct CharacterData {
    std::string name;                  // Character's name
    std::string portraitPath;          // Path to the character's portrait texture
    wolf::Texture* portraitTexture = nullptr;  // Pointer to the loaded texture
};

// Structure to hold each dialogue line data
struct DialogueLine {
    std::string characterName;         // Name of the character speaking (optional)
    std::string text;                  // Dialogue text (optional)
    float duration = 0.0f;             // Duration for which the line is displayed
    std::string action;                // Action type, e.g., "cutscene" (optional)
    std::string cutsceneID;            // Cutscene ID if action is "cutscene" (optional)
    std::string target;                // Camera target if action involves movement (optional)
};

// Structure to hold the entire dialogue data
struct DialogueData {
    std::vector<CharacterData> characters;  // List of characters in the dialogue
    std::vector<DialogueLine> lines;        // List of dialogue lines
};

// Dialogue Manager Class
class DialogueManager {
public:
    DialogueManager() = default;
    ~DialogueManager();

    // Load dialogues from a YAML file
    void LoadDialogueFromYAML(const std::string& filePath);

    // Retrieve a pointer to dialogue data by ID
    DialogueData* GetDialogue(const std::string& id);

    // Retrieve dialogue lines by ID
    const std::vector<DialogueLine>& GetDialogueLinesById(const std::string& id) const;

    // Get character portrait texture by name
    wolf::Texture* GetCharacterPortraitTexture(const std::string& characterName);

private:
    std::unordered_map<std::string, DialogueData> m_dialogues; // Map of dialogues by ID
};