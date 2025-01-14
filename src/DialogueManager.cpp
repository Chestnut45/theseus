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
        // Load the YAML file
        YAML::Node config = YAML::LoadFile(filePath);

        // Iterate through the "dialogue" entries in the YAML file
        for (const auto& dialogueNode : config["dialogue"])
        {
            DialogueData dialogueData;
            std::string id;

            // Ensure the "id" field exists
            if (dialogueNode["id"]) {
                id = dialogueNode["id"].as<std::string>();
            } else {
                std::cerr << "Dialogue entry missing 'id' field, skipping..." << std::endl;
                continue;
            }

            // Parse characters in the dialogue
            for (const auto& characterNode : dialogueNode["characters"])
            {
                CharacterData character;
                character.name = characterNode["name"].as<std::string>();
                character.portraitPath = characterNode["portraitPath"].as<std::string>();

                // Load portrait texture
                character.portraitTexture = wolf::TextureManager::CreateTexture(character.portraitPath);
                dialogueData.characters.push_back(character);
            }

            // Parse dialogue lines
            for (const auto& lineNode : dialogueNode["lines"])
            {
                DialogueLine line;

                // Optional character and text fields
                if (lineNode["character"]) {
                    line.characterName = lineNode["character"].as<std::string>();
                }
                if (lineNode["text"]) {
                    line.text = lineNode["text"].as<std::string>();
                }
                if (lineNode["duration"]) {
                    line.duration = lineNode["duration"].as<float>();
                }

                // Handle actions like "cutscene" or other events
                if (lineNode["action"]) {
                    line.action = lineNode["action"].as<std::string>();
                }
                if (lineNode["cutsceneID"]) {
                    line.cutsceneID = lineNode["cutsceneID"].as<std::string>();
                }

                dialogueData.lines.push_back(line);
            }

            // Store dialogue data in the map
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