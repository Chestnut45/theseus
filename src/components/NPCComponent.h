#pragma once

//-----------------------------------------------------------------------------
// File:            NPCComponent.h
// Original Author: Aurora Ryder
//
// A class representing a Non-Player Character (NPC)
//-----------------------------------------------------------------------------

#include <W_BaseComponent.h>
#include <string>

// --------------- Back-end ---------------
// [ ] - Component Setup
// [x] - Name
// [ ] - Dialogue
// [x] - Merchant inventory (optional)
// [x] - Drop table
// [ ] - Collider (handled separately)
// [ ] - Health (handled separately)
// [ ] - Spawning
// ----------------------------------------

// -------------- Data-Drive --------------
// [ ] - Dialogue
// [ ] - Merchant inventory
// [ ] - Drop table
// [ ] - Whole Entity Setup
//       [ ] - Collider Component
//       [ ] - Health Component
//       [ ] - NPC Component
//       [ ] - AnimatedSprite2D Component
// ----------------------------------------

// Struct used to sort all of the NPC's possible dialogues into a minimum priority queue
// so that we can programmatically control the ordering and repeatability of each conversation
struct NPCDialogueEntry {
    int iPriority; // The entry with the lowest value will always play FIRST and the lowest possible value is 0 (zero)
    std::string strDialogueID; // This id MUST correspond to a YAML Node in the dialogue file the NPC is drawing from

    bool bHasPlayed; // Has this dialogue played yet?
    bool bCanRepeat; // Can this dialogue repeat (play multiple times)?

    // !-- Note that if a dialogue entry repeats, its priority value will be changed to one level higher
    // than the current highest dialogue value after it has been played for the first time to ensure
    // that replayable dialogue does not take priority over new or unique entries --!
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

    private:
        int m_iID;
        static int m_iNextID;

        std::string m_strName;
        std::string m_strDialogueFilePath;
        std::string m_strDropTableFilePath;

        bool m_bIsMerchant;     // Is this NPC currently a merchant? (they have a currently accessible MerchantInventoryComponent)
        const bool m_bCanBeMerchant;  // Can this NPC be a merchant? (they have a MerchantInventoryComponent that is -- or will be -- accessible)
};