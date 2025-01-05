#pragma once

#include <string>

// Event structure to carry the ID of the sequence to be triggered
struct DialogueAndCutsceneEvent {
    DialogueAndCutsceneEvent(const std::string& id, const std::string& file) : sequenceID(id), dialogueFilePath(file) {}

    std::string sequenceID;  // ID of the dialogue or cutscene sequence to be triggered
    std::string dialogueFilePath; // .yaml file that holds the dialogue or cutscene sequence the id corresponds to
};
