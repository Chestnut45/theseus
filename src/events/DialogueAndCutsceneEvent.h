#pragma once

#include <string>

// Event structure to carry the ID of the sequence to be triggered
struct DialogueAndCutsceneEvent {
    DialogueAndCutsceneEvent(const std::string& id) : sequenceID(id) {}

    std::string sequenceID;  // ID of the dialogue or cutscene sequence to be triggered
};
