#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

// Structure to hold character-specific data
struct CharacterData {
    std::string name;
    std::string portraitPath;
    std::string expression;
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
    ~DialogueManager() = default;

    // Load dialogues from a YAML file
    void LoadDialogueFromYAML(const std::string& filePath);

    // Retrieve a pointer to dialogue data by ID
    DialogueData* GetDialogue(const std::string& id);

    // Retrieve dialogue lines by ID
    const std::vector<DialogueLine>& GetDialogueLinesById(const std::string& id) const;

private:
    std::unordered_map<std::string, DialogueData> m_dialogues; // Map of dialogues by ID
};