#pragma once
#include <string>

struct DialogueResumeEvent {
    explicit DialogueResumeEvent(const std::string& cutsceneID)
        : m_cutsceneID(cutsceneID) {}

    std::string m_cutsceneID;  // The ID of the cutscene that has just completed
};
