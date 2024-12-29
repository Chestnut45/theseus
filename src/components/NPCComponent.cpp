#include "NPCComponent.h"

int NPCComponent::m_iNextID = 0;

NPCComponent::NPCComponent(const std::string& p_strName, const std::string& p_strDialogueFilePath, const std::string& p_strDropTableFilePath, bool p_bIsMerchant, bool p_bCanBeMerchant)
    : m_strName(p_strName), m_strDialogueFilePath(p_strDialogueFilePath), m_strDropTableFilePath(p_strDropTableFilePath), m_bCanBeMerchant(p_bCanBeMerchant)
{
    // If the NPC has the ability to be a merchant
    if (p_bCanBeMerchant) {
        // Then we let the user choose whether or not they START as a merchant
        m_bIsMerchant = p_bIsMerchant;
    }
    else {
        // Otherwise, we want to make sure that the NPC doesn't show as being a merchant
        // when the rest of the code is not treating them as one.
        // (We want to avoid the case where CanBeMerchant == FALSE but IsMerchant == TRUE)
        m_bIsMerchant = false;
    }

    // Parse the dialogue
    

    // Assign a unique ID number to this NPC and update the NextID counter
    m_iID = m_iNextID;
    m_iNextID += 1;
}

NPCComponent::~NPCComponent() {
    // Empty the dialogue queue
    this->EmptyDialogueQueue();

    // And delete all of the dialogue entries
    m_mDialogueEntries.clear();
}

// Play the dialogue entry with the lowest priority value and remove it from the queue
// (Note that replayable entries are re-added to the queue with a higher priority value
//  so that they will play AFTER new or unique dialogue)
void NPCComponent::PlayNextDialogue() {
    // Take the top element off of the queue
    NPCDialogueEntry* dialogue = m_pqDialogueQueue.top();
    m_pqDialogueQueue.pop();

    // Play the dialogue

    // If the dialogue can be replayed
    if (dialogue->bCanRepeat) {
        // Add it back into the queue with a higher priority so that it will be at the back of the queue
        m_iCurHighPriorityVal += 1;
        dialogue->iPriority = m_iCurHighPriorityVal;
        m_pqDialogueQueue.push(dialogue);
    }
}

// Add a dialogue entry to the priority queue
void NPCComponent::QueueDialogue(const std::string& p_strEntryID) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        if (!dialogue->bHasPlayed || dialogue->bHasPlayed && dialogue->bCanRepeat) {
            m_pqDialogueQueue.push(dialogue);

            // If the priority value of that item was higher than the current highest
            if (dialogue->iPriority > m_iCurHighPriorityVal) {
                // Set it as the current highest
                m_iCurHighPriorityVal = dialogue->iPriority;
            }
        }
    }
}

// Check whether or not a given dialogue entry has been played yet
bool NPCComponent::HasDialoguePlayed(const std::string& p_strEntryID) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        return dialogue->bHasPlayed;
    }
    return false;
}

// Change the priority level of a given dialogue entry
// ?-- Need to figure out how this will work with the priority queue --?
bool NPCComponent::ChangeDialoguePriority(const std::string& p_strEntryID, int p_iNewPriority) {
    NPCDialogueEntry* dialogue = m_mDialogueEntries.at(p_strEntryID);
    if (dialogue) {
        dialogue->iPriority = p_iNewPriority;
        return true;
    }
    return false;
}

// Wrapper method to empty the dialogue queue
void NPCComponent::EmptyDialogueQueue() {
    while (!m_pqDialogueQueue.empty()) {
        m_pqDialogueQueue.pop();
    }
}