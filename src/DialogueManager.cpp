#include "DialogueManager.h"
#include <iostream>

DialogueManager::~DialogueManager() {
    // Clean up textures (if needed)
    for (auto& dialoguePair : m_dialogues) {
        for (auto& character : dialoguePair.second.characters) {
            if (character.portraitTexture) {
                wolf::TextureManager::DestroyTexture(character.portraitTexture);
            }
        }
    }
}


// Function to load dialogue data from a YAML file
void DialogueManager::LoadDialogueFromYAML(const std::string& filePath)
{
    std::cout << "Loading dialogue from YAML file: " << filePath << std::endl;
    try {
        YAML::Node config = YAML::LoadFile(filePath);
        for (const auto& dialogueNode : config["dialogue"])
        {
            DialogueData dialogueData;
            std::string id = dialogueNode["id"].as<std::string>();

            // Parse characters in the dialogue
            for (const auto& characterNode : dialogueNode["characters"])
            {
                CharacterData character;
                character.name = characterNode["name"].as<std::string>();
                character.portraitPath = characterNode["portraitPath"].as<std::string>();
                character.expression = characterNode["expression"].as<std::string>();

                // Load the texture for the portrait
                character.portraitTexture = wolf::TextureManager::CreateTexture(character.portraitPath);
                
                dialogueData.characters.push_back(character);
            }

            // Parse each line in the dialogue
            for (const auto& lineNode : dialogueNode["lines"])
            {
                DialogueLine line;
                line.characterName = lineNode["character"].as<std::string>();
                line.text = lineNode["text"].as<std::string>();
                line.duration = lineNode["duration"].as<float>();
                dialogueData.lines.push_back(line);
            }

            // Store the dialogue data in the map
            m_dialogues[id] = dialogueData;
        }
        std::cout << "Successfully loaded dialogue data from: " << filePath << std::endl;
    }
    catch (const YAML::Exception& e) {
        std::cerr << "Error loading YAML file: " << e.what() << std::endl;
    }
}

// Function to retrieve a pointer to dialogue data by ID
DialogueData* DialogueManager::GetDialogue(const std::string& id)
{
    auto it = m_dialogues.find(id);
    if (it != m_dialogues.end()) {
        return &it->second;
    }
    return nullptr;
}

// Function to retrieve dialogue lines by ID
const std::vector<DialogueLine>& DialogueManager::GetDialogueLinesById(const std::string& id) const
{
    // Return the lines of the dialogue if found; otherwise, return an empty vector
    static const std::vector<DialogueLine> emptyLines = {};

    auto it = m_dialogues.find(id);
    if (it != m_dialogues.end()) {
        return it->second.lines;
    }

    std::cerr << "Dialogue ID " << id << " not found!" << std::endl;
    return emptyLines;
}

wolf::Texture* DialogueManager::GetCharacterPortraitTexture(const std::string& characterName) {
    for (const auto& dialogue : m_dialogues) {
        for (const auto& character : dialogue.second.characters) {
            if (character.name == characterName) {
                return character.portraitTexture;
            }
        }
    }
    return nullptr;  // Return nullptr if texture not found
}