#pragma once

#include <string>

// Event structure to carry the dialogue ID that should be triggered
struct DialogueTriggerEvent
{
    DialogueTriggerEvent(const std::string& id) : dialogueID(id) {}

    std::string dialogueID;  // ID of the dialogue to be triggered
};