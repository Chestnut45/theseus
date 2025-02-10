#include "NPCBuilder.h"
#include <unordered_map>

NPCBuilder* NPCBuilder::m_pInstance = nullptr;
wolf::Scene* NPCBuilder::m_pScene = nullptr;
wolf::RNG* NPCBuilder::m_pRNG = nullptr;

int NPCBuilder::m_iRNGSeed;

const std::string NPCBuilder::NPC_DIRECTORY_PATH = "data/npc_directory.yaml";
const ImVec2 NPCBuilder::NPC_INVENTORY_DRAW_POS = {800.0f, 200.0f};

void NPCBuilder::CreateInstance(wolf::Scene* p_pScene, int p_iRNGSeed) {
    // If there is not already an existing instance
    if (!m_pInstance) {
        // Create one
        m_pInstance = new NPCBuilder();
        m_pRNG = new wolf::RNG(p_iRNGSeed);
        m_pScene = p_pScene;
        m_iRNGSeed = p_iRNGSeed;
    }
}

void NPCBuilder::DestroyInstance() {
    // If an instance exists
    if (m_pInstance) {
        // Delete it
        delete(m_pInstance);
        delete(m_pRNG);

        m_pInstance = nullptr;
        m_pScene = nullptr;
        m_pRNG = nullptr;
    }
}

NPCBuilder& NPCBuilder::Instance() {
    if (!m_pInstance) {
        m_pInstance = new NPCBuilder();
    }

    return *m_pInstance;
}

void NPCBuilder::SetScene(wolf::Scene* p_pScene) {
    m_pScene = p_pScene;
}


void NPCBuilder::SetSeed(int p_iRNGSeed) {
    m_iRNGSeed = p_iRNGSeed;
}

// Use this method when you know which NPC you want to build and can provide
// the corresponding .yaml file
wolf::GameObject* NPCBuilder::BuildNPC(const std::string& p_strFilePath) {
    wolf::GameObject* pConstructedNPC = &m_pScene->CreateObject2D();
    std::unordered_map<std::string, NPCDialogueEntry*> m_mDialogueMap;

    try {
        // First load up the npc file
        YAML::Node pNPCDetails = YAML::LoadFile(p_strFilePath);

        // Then start looking for the attributes we need to create an NPCComponent
        std::string strName = pNPCDetails["name"].as<std::string>();
        std::string strLootTable = pNPCDetails["loot_table_path"].as<std::string>();

        // This is the file that we'll be drawing from anytime the NPC is presenting dialogue or a cutscene
        std::string strDialogueBank = pNPCDetails["dialogue_bank_path"].as<std::string>();

        // We'll need to iterate through the list of dialogue/cutscene entries and create a map of 'em
        // with the entry id (or name) as the key and the rest of the information as the value
        YAML::Node dialogueEntries = pNPCDetails["dialogue_entries"];
        for (int e = 0; e < dialogueEntries.size(); ++e) {
            int priority = dialogueEntries[e]["priority"].as<int>();            // Priority value (lower values play before higher ones)
            std::string id = dialogueEntries[e]["entry_id"].as<std::string>();  // Entry ID that matches an entry in the dialogue bank
            bool hasTrigger = dialogueEntries[e]["has_trigger"].as<bool>();     // Whether or not this entry must be triggered to play
            bool canRepeat = dialogueEntries[e]["can_repeat"].as<bool>();       // Whether or not this entry can play multiple times

            // Create a new entry object and add it to the map
            NPCDialogueEntry* pEntry = new NPCDialogueEntry(priority, id, hasTrigger, canRepeat);
            m_mDialogueMap.insert({id, pEntry});
        }

        // Check whether or not this NPC can be a merchant
        bool bCanBeMerchant = pNPCDetails["can_be_merchant"].as<bool>();

        // This attribute may not be present so we use a lambda expression to default to false when it isn't there
        bool bStartsAsMerchant = pNPCDetails["starts_as_merchant"] ? pNPCDetails["starts_as_merchant"].as<bool>() : false;

        // Create the NPCComponent and attach it to the in-progress GameObject
        auto& npcComp = pConstructedNPC->AddComponent<NPCComponent>(strName, strDialogueBank, m_mDialogueMap, strLootTable, bStartsAsMerchant, bCanBeMerchant);

        // If this NPC can be a merchant then we'll need to look for the attributes
        // required to set up a MerchantInventoryComponent
        if (bCanBeMerchant) {
            // Get the merchant node
            YAML::Node merchantInit = pNPCDetails["merchant_init"];
            
            // Get the inventory dimensions
            int iInvSize = merchantInit["inventory_size"].as<int>();
            int iSlotsPerRow = merchantInit["max_slots_per_row"].as<int>();

            // Get the markup percentage and starting gold amount
            float fPercentMarkup = merchantInit["percent_markup"].as<float>();
            int iGold = merchantInit["starting_gold"].as<int>();

            // Get the file that lists all of the items this merchant will sell
            std::string strMerchantLootFile = merchantInit["merchant_loot_file"].as<std::string>();

            // Create and attach a MerchantInventoryComponent to the GameObject
            MerchantInventoryComponent& pMerchantInv = pConstructedNPC->AddComponent<MerchantInventoryComponent>(iInvSize, iSlotsPerRow, NPC_INVENTORY_DRAW_POS, strName, fPercentMarkup, iGold);
            
            // Then fill the inventory using the loot file we retrieved a moment ago
            pMerchantInv.FillInventoryFromFile(strMerchantLootFile);
        }

        // Next we need to create the health component
        int iHealth = pNPCDetails["health"].as<int>();
        pConstructedNPC->AddComponent<HealthComponent>(iHealth);

        // As well as a Velocity Component
        pConstructedNPC->AddComponent<VelocityComponent>();

        // Then we look for the collider attributes...
        glm::vec2 v2Size;
        v2Size.x = pNPCDetails["collider_size"]["x"].as<float>();
        v2Size.y = pNPCDetails["collider_size"]["y"].as<float>();

        glm::vec2 v2Offset;
        v2Offset.x = pNPCDetails["collider_offset"]["x"].as<float>();
        v2Offset.y = pNPCDetails["collider_offset"]["y"].as<float>();

        // ...before creating and attaching a collider to the in-progress GameObject
        auto& pCollider = pConstructedNPC->AddComponent<ColliderComponent>(ColliderComponent::HITHURTBOXDR, false, true);
        pCollider.AddColliderBox(v2Size, v2Offset);

        // Finally, we look for the attributes needed to create an AnimatedSprite2D
        YAML::Node animInit = pNPCDetails["animation_init"];
        
        // Get the size of the frames
        glm::vec2 v2FrameSize;
        v2FrameSize.x = animInit["frame_size"]["x"].as<float>();
        v2FrameSize.y = animInit["frame_size"]["y"].as<float>();

        // Get the playback speed, texture path, and name of the starting animation
        float fPlaybackSpeed = animInit["playback_speed"].as<float>();
        std::string strTexturePath = animInit["texture"].as<std::string>();
        std::string strStartAnim = animInit["start_anim"].as<std::string>();

        // Use that information to create and attach an AnimatedSprite2D
        auto& pAnim = pConstructedNPC->AddComponent<AnimatedSprite2D>(strTexturePath, v2FrameSize, fPlaybackSpeed);

        // Then loop through all of the animation sets
        YAML::Node animSets = animInit["animation_sets"];
        for (int i = 0; i < animSets.size(); ++i) {
            // And add each to the component we just created
            std::string strAnimSetFilePath = animSets[i].as<std::string>();
            pAnim.AddAnimationSet(strAnimSetFilePath);
        }

        // Then set the component's current animation to be the starting animation
        pAnim.SetAnimation(strStartAnim);

        // Finally, we initialize the NPC
        npcComp.Init();
    }
    catch (YAML::Exception e) { // If we run into an issue while parsing the file
        // Report the error
        wolf::Error("Error using '", p_strFilePath.c_str(), "' to construct NPC: ", e.what());

        // Delete anything that was stored in the dialogue map
        m_mDialogueMap.clear();

        // Delete the in-progress GameObject
        m_pScene->DeleteObject(pConstructedNPC->GetID());

        // And return nullptr
        return nullptr;
    }

    // If nothing went wrong, we're good to return the GameObject
    return pConstructedNPC;
}

wolf::GameObject* NPCBuilder::BuildRandomNPC() {
    // Variable to hold the npc we create
    wolf::GameObject* pRandomNPC = nullptr;

    try {
        // Load up the NPC directory file
        YAML::Node baseNode = YAML::LoadFile(NPC_DIRECTORY_PATH);

        // And grab the directory node
        YAML::Node directory = baseNode["directory"];
        int numEntries = directory.size();

        // Initialize an array of entries
        YAML::Node entries[numEntries];

        // Count the number of items in the loot table
        float sumWeights = 0;
        for (int i = 0; i < numEntries; ++i)
        {
            entries[i] = directory[i];
            const YAML::Node& entry = entries[i];
            sumWeights += entry["probability"].as<float>();
        }

        // Generate a random number
        float value = m_pRNG->NextFloat(0.0f, sumWeights);
        std::string chosenInitFile;
        for (int j = 0; j < numEntries; ++j)
        {
            float probability = entries[j]["probability"].as<float>();
            
            if (value < probability)
            {
                // And once we've chosen an npc, retrieve the init file associated with it
                chosenInitFile = entries[j]["init_file"].as<std::string>();
                break;
            }
            value -= probability;
        }

        // Then create an npc using that file
        pRandomNPC = this->BuildNPC(chosenInitFile);
    }
    catch (YAML::Exception e) {
        // If we run into an error, then we should print it and return nullptr
        wolf::Error("YAML: Issue with ", NPC_DIRECTORY_PATH, ": ", e.what());
        return nullptr;
    }

    // And return what we created (note that if the BuildNPC method ran into an error, this will return nullptr)
    return pRandomNPC;
};