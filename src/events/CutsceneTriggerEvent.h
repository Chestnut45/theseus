#pragma once
#include <string>

struct CutsceneTriggerEvent
{
    std::string cutsceneID;    // ID of the cutscene to trigger

    CutsceneTriggerEvent(const std::string& cutsceneID)
        : cutsceneID(cutsceneID) {}
};
