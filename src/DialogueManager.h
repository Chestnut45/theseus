#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "W_Texture.h"
#include "W_TextureManager.h"

// Structure to hold character-specific data
struct CharacterData {
    std::string name;
    std::string portraitPath;
    std::string expression;
    wolf::Texture* portraitTexture = nullptr;  // Pointer to texture
};

// Structure to hold each dialogue line data
struct DialogueLine {
    std::string characterName;
    std::string text;
    float duration;
};

// Structure to hold the entire dialogue data
struct DialogueData {
    std::vector<CharacterData> characters;
    std::vector<DialogueLine> lines;
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