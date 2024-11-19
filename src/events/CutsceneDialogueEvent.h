#pragma once
#include <string>

struct CutsceneDialogueEvent
{
    std::string dialogueID;    // ID of the dialogue to trigger
    std::string cutsceneID;    // ID of the cutscene to trigger

    CutsceneDialogueEvent(const std::string& dialogueID, const std::string& cutsceneID)
        : dialogueID(dialogueID), cutsceneID(cutsceneID) {}
};
