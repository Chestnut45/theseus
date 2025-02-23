#pragma once
#include <string>

// Created by Aurora Ryder for use with NPCs and the Dialogue/Cutscene system

// Event to carry the sequence and optionally the npc id of a dialogue or cutscene sequence that has just ended
struct DialogueOrCutsceneEndEvent {
    DialogueOrCutsceneEndEvent(const std::string& p_sequenceID, int p_npcID) : sequenceID(p_sequenceID), triggerNPCID(p_npcID) {};
    DialogueOrCutsceneEndEvent(const std::string& p_sequenceID) : sequenceID(p_sequenceID) {};

    std::string sequenceID;  // ID of the dialogue or cutscene sequence that was triggered
    int triggerNPCID = -1;  // Optional ID of the npc who triggered the dialogue or cutscene sequence
};