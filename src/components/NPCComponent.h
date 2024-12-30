#pragma once

//-----------------------------------------------------------------------------
// File:            NPCComponent.h
// Original Author: Aurora Ryder
//
// A class representing a Non-Player Character (NPC)
//-----------------------------------------------------------------------------

#include <W_BaseComponent.h>
#include <string>
#include <queue>
#include <map>

#include <W_Transform2D.h>
#include <ItemDropCreator.h>
#include <HealthComponent.h>
#include <AnimatedSprite2D.h>
#include <MerchantInventoryComponent.h>

// --------------- Back-end ---------------
// [x] - Component Setup
// [x] - Name
// [?] - Dialogue
// [ ] - Merchant inventory (optional)
// [x] - Drop table
// [x] - Collider (handled separately)
// [x] - Health (handled separately)
// [ ] - Spawning
// [x] - Handle death and item dropping
// [ ] - Velocity Component
// ----------------------------------------

// -------------- Data-Drive --------------
// [ ] - Dialogue
// [x] - Merchant inventory
// [ ] - Drop table
// [ ] - Whole Entity Setup
//       [x] - Collider Component
//       [x] - Health Component
//       [ ] - NPC Component
//       [x] - AnimatedSprite2D Component
// ----------------------------------------

// Struct used to sort all of the NPC's possible dialogues into a minimum priority queue
// so that we can programmatically control the ordering and repeatability of each conversation
struct NPCDialogueEntry {
    NPCDialogueEntry(int p_iPriority, const std::string& p_strDialogueID, bool p_bHasTrigger, bool p_bCanRepeat)
    : iPriority(p_iPriority), strDialogueID(p_strDialogueID), bHasTrigger(p_bHasTrigger), bCanRepeat(p_bCanRepeat)
    {};

    int iPriority; // The entry with the lowest value will always play FIRST and the lowest possible value is 0 (zero)
    std::string strDialogueID; // This id MUST correspond to a YAML Node in the dialogue file the NPC is drawing from

    bool bHasPlayed = false; // Has this dialogue been played yet?

    bool bHasTrigger; // Can this dialogue play immediately or does it have to be triggered by something?
    bool bCanRepeat; // Can this dialogue repeat (play multiple times)?

    // !-- Note that if a dialogue entry repeats, its priority value will be changed to one level higher
    // than the current highest dialogue value after it has been played for the first time to ensure
    // that replayable dialogue does not take priority over new or unique entries. Also note that a
    // repeatable dialogue will never leave permanently leave the queue once it has been added as it
    // will be continually added to the end of the queue everytime it plays --!
};

// Function to compare the priority level of NPCDialogueEntries
struct ComparePriority {
    bool operator()(const NPCDialogueEntry* a, const NPCDialogueEntry* b) {
        return a->iPriority > b->iPriority;
    }
};

class NPCComponent : public wolf::BaseComponent {
    public:
        NPCComponent(const std::string& p_strName, const std::string& p_strDialogueFilePath, const std::string& p_strDropTableFilePath, bool p_bIsMerchant, bool p_bCanBeMerchant);
        ~NPCComponent();
        
        // Delete copy constructor/assignment
        NPCComponent(const NPCComponent&) = delete;
        NPCComponent& operator=(const NPCComponent&) = delete;

        // Delete move constructor/assignment
        NPCComponent(NPCComponent&& other) = delete;
        NPCComponent& operator=(NPCComponent&& other) = delete;

        int GetID() const {return m_iID;};

        void Init();
        void Update(float p_fDelta);
        void HandleDeadState(float p_fDelta);

        const std::string& GetName() const {return m_strName;};
        void SetName(const std::string& p_strName) {m_strName = p_strName;};

        const std::string& GetDialogueFilePath() const {return m_strDialogueFilePath;};
        void SetDialogueFilePath(const std::string& p_strFilePath) {m_strDialogueFilePath = p_strFilePath;};

        const std::string& GetDropTableFilePath() const {return m_strDropTableFilePath;};
        void SetDropTableFilePath(const std::string& p_strFilePath) {m_strDropTableFilePath = p_strFilePath;};

        // If the NPC's GameObject has a MerchantInventoryComponent attached, is it currently accessible?
        bool IsMerchant() const {return m_bIsMerchant;};

        // If the NPC's GameObject has a MerchantInventoryComponent attached, toggle its accessiblity.
        // Return true if the accessibility was toggled and false otherwise
        bool SetIsMerchant(bool p_bIsMerchant) {
            // If this NPC has the ability to become (or stop being) a merchant
            if (m_bCanBeMerchant) {
                // Toggle their merchant-hood
                m_bIsMerchant = p_bIsMerchant;

                // And return true
                return true;
            }

            // Otherwise, this NPC can NEVER be a merchant so we don't toggle IsMerchant and return false
            return false;
        };
        
        // Does this NPC's GameObject have a MerchantInventoryComponent attached?
        bool CanBeMerchant() const {return m_bCanBeMerchant;};

        // Dialogue and priority queue operations
        void PlayNextDialogue();
        void QueueDialogue(const std::string& p_strEntryID);

        bool HasDialoguePlayed(const std::string& p_strEntryID);
        bool ChangeDialoguePriority(const std::string& p_strEntryID, int p_iNewPriority);

        void EmptyDialogueQueue();

    private:
        int m_iID;
        static int m_iNextID;

        std::string m_strName;
        std::string m_strDialogueFilePath;
        std::string m_strDropTableFilePath;

        bool m_bIsMerchant; // Is this NPC currently a merchant? (they have a currently accessible MerchantInventoryComponent)
        const bool m_bCanBeMerchant;  // Can this NPC be a merchant? (they have a MerchantInventoryComponent that is -- or will be -- accessible)

        std::map<std::string, NPCDialogueEntry*> m_mDialogueEntries; // Map to hold all of the NPC's dialogues
        std::priority_queue<NPCDialogueEntry*, std::vector<NPCDialogueEntry*>, ComparePriority> m_pqDialogueQueue; // Priority queue to decide which dialogue will play next

        int m_iCurHighPriorityVal = 0;

        // Timers for the NPC death animation
        float m_fFallDeadTimer = 0.0f;
        float m_fLieDeadTimer = 0.0f;
        float m_fTimeToFallDead = 0.6f;
        float m_fTimeToLieDead = 0.8f;

        // Pointers to other components that the NPCComponent will occasionally need to access
        HealthComponent* m_pHealthComp = nullptr;
        wolf::Transform2D* m_pTransform = nullptr;
        AnimatedSprite2D* m_pAnimSpriteComp = nullptr;
        MerchantInventoryComponent* m_pMerchInvComp = nullptr;
};