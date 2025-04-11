#pragma once

//-----------------------------------------------------------------------------
// File:            NPCComponent.h
// Original Author: Aurora Ryder
// Modifications: Nguyễn Minh Nhật
// A class representing a Non-Player Character (NPC)
//-----------------------------------------------------------------------------

#include <W_BaseComponent.h>
#include <string>
#include <queue>
#include <unordered_map>

#include <W_Transform2D.h>
#include <W_EventManager.h>
#include <W_RNG.h>
#include <HealthComponent.h>
#include <AnimatedSprite2D.h>
#include <MerchantInventoryComponent.h>
#include <VelocityComponent.h>

#include <ItemDropCreator.h>
#include <DialogueAndCutsceneEvent.h>
#include <DialogueOrCutsceneEndEvent.h>

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
    inline bool operator()(const NPCDialogueEntry* a, const NPCDialogueEntry* b) {
        return a->iPriority > b->iPriority;
    }
};

class NPCComponent : public wolf::BaseComponent {
    public:
        enum State
        {
            IDLE,
            ROAM,
            STUNNED,
            DEAD
        };

        NPCComponent(const std::string& p_strName, const std::string& p_strDialogueFilePath, std::unordered_map<std::string, NPCDialogueEntry*>& p_mDialogueEntries, const std::string& p_strDropTableFilePath, bool p_bIsMerchant, bool p_bCanBeMerchant);
        ~NPCComponent();
        
        // Delete copy constructor/assignment
        NPCComponent(const NPCComponent&) = delete;
        NPCComponent& operator=(const NPCComponent&) = delete;

        // Delete move constructor/assignment
        NPCComponent(NPCComponent&& other) = delete;
        NPCComponent& operator=(NPCComponent&& other) = delete;

        inline int GetID() const {return m_iID;};

        void Init();
        void Update(float p_fDelta);
        void HandleDeadState(float p_fDelta);

        inline const std::string& GetName() const {return m_strName;};
        inline void SetName(const std::string& p_strName) {m_strName = p_strName;};

        inline const std::string& GetDialogueFilePath() const {return m_strDialogueFilePath;};
        inline void SetDialogueFilePath(const std::string& p_strFilePath) {m_strDialogueFilePath = p_strFilePath;};

        inline const std::string& GetDropTableFilePath() const {return m_strDropTableFilePath;};
        inline void SetDropTableFilePath(const std::string& p_strFilePath) {m_strDropTableFilePath = p_strFilePath;};

        // If the NPC's GameObject has a MerchantInventoryComponent attached, is it currently accessible?
        inline bool IsMerchant() const {return m_bIsMerchant;};

        // If the NPC's GameObject has a MerchantInventoryComponent attached, toggle its accessiblity.
        // Return true if the accessibility was toggled and false otherwise
        inline bool SetIsMerchant(bool p_bIsMerchant) {
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
        inline bool CanBeMerchant() const {return m_bCanBeMerchant;};

        // Dialogue and priority queue operations
        void PlayNextDialogue();
        void QueueDialogue(const std::string& p_strEntryID);
        void TriggerDialogue(const std::string& p_strEntryID);

        bool HasDialoguePlayed(const std::string& p_strEntryID);
        bool ChangeDialoguePriority(const std::string& p_strEntryID, int p_iNewPriority);

        void EmptyDialogueQueue();
        void SayGoodbye();
        
        // Changes NPC state to STUNNED
        void StunNPC();

        // Sets the active flag
        void SetActive(bool p_active);

        //youssef moved this to public
        void ChangeState(State p_state);


    private:
        void HandleDialogueOrCutsceneEndEvent(const DialogueOrCutsceneEndEvent& p_event);

        int m_iID;
        static int m_iNextID;

        std::string m_strName;
        std::string m_strDialogueFilePath;
        std::string m_strDropTableFilePath;

        bool m_bIsMerchant; // Is this NPC currently a merchant? (they have a currently accessible MerchantInventoryComponent)
        bool m_bIsMerching; // If this merchant NPC is pushing their ill-gotten wares to their unsuspecting customers
        const bool m_bCanBeMerchant;  // Can this NPC be a merchant? (they have a MerchantInventoryComponent that is -- or will be -- accessible)

        std::unordered_map<std::string, NPCDialogueEntry*> m_mDialogueEntries; // Map to hold all of the NPC's dialogues
        std::priority_queue<NPCDialogueEntry*, std::vector<NPCDialogueEntry*>, ComparePriority> m_pqDialogueQueue; // Priority queue to decide which dialogue will play next

        int m_iCurHighPriorityVal = 0;

        bool m_bPlayingDialogue = false; // Flag to check if the NPC can be talked to / interacted with
        State m_state = State::IDLE; //-------Added By Nhat-------//

        // Idle state members
        float m_fIdleTimer = 0.0f;
        
        // Roam state members
        float m_fRoamTimer = 0.0f;
        float m_fRoamSpeedCheckTime = 0.2f;
        float m_fRoamSpeedCheckTimer = 0.0f;
        float m_fRoamSpeed = 100.0f;
        float m_RoamSpeedMin = 50.0f; // The minimum speed that determines if the NPC should adjust their velocity
        int m_iRoamBlockedCounter = 0; // Counts how many times the NPC walks into a wall or corner and is blocked - reset in EnterIdleState()
        int m_iRoamBlockedLimit = 2; // The mumber of blocks allowed before the NPC is forced into IDLE

        // Stunned state members
        float m_fStunnedTime = 0.5f;
        float m_fStunnedTimer = 0.0f;

        // Timers for the NPC death animation
        float m_fFallDeadTimer = 0.0f;
        float m_fLieDeadTimer = 0.0f;
        float m_fTimeToFallDead = 0.6f;
        float m_fTimeToLieDead = 0.8f;

        // RNG generator
        static wolf::RNG s_RNG;

        // Active flag - stops updating when the NPC is in a deactivated chunk
        bool m_isActive = true;

        // The ID of the chunk the NPC is in
        glm::ivec2 m_chunkID;

        // Pointers to other components that the NPCComponent will occasionally need to access
        HealthComponent* m_pHealthComp = nullptr;
        wolf::Transform2D* m_pTransform = nullptr;
        AnimatedSprite2D* m_pAnimSpriteComp = nullptr;
        MerchantInventoryComponent* m_pMerchInvComp = nullptr;
        VelocityComponent* m_pVeloComp = nullptr;

        // State entry methods
        void EnterIdleState();
        void EnterRoamState();
        void EnterStunnedState();

        // State handling methods
        void HandleIdleState(float p_fDelta);
        void HandleRoamState(float p_fDelta);
        void HandleStunnedState(float p_fDelta);

        // State exit methods
        void ExitStunnedState();

        // Adjusts NPC velocity in cases where the NPC is roaming too slowly - such as constantly colliding with a wall
        void CheckRoamSpeed();
        
        // Methods for turning NPCs
        void TurnTowardsPlayer();
        void TurnToDirection(glm::vec2 p_vDirection);
};