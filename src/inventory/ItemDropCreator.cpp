#include "ItemDropCreator.h"

ItemDropCreator* ItemDropCreator::m_pInstance = nullptr;
wolf::Scene* ItemDropCreator::m_pScene = nullptr;
wolf::RNG* ItemDropCreator::m_pRNG = nullptr;

int ItemDropCreator::m_iRNGSeed;
std::map<std::string, YAML::Node> ItemDropCreator::m_mLootTables;

const std::string ItemDropCreator::ITEM_TEXTURE_PATH = "data/textures/ItemIcons-Sheet.png";

void ItemDropCreator::CreateInstance(wolf::Scene* p_pScene, int p_iRNGSeed) {
    // If there is not already an existing instance
    assert(m_pInstance == nullptr);

    // Create one
    m_pInstance = new ItemDropCreator();
    m_pRNG = new wolf::RNG(p_iRNGSeed); // This seed value is completely arbitrary
    m_pScene = p_pScene;
    m_iRNGSeed = p_iRNGSeed;
}

void ItemDropCreator::DestroyInstance() {
    // If an instance exists
    assert(m_pInstance != nullptr);

    // Delete it
    delete(m_pInstance);
    delete(m_pRNG);

    m_pInstance = nullptr;
    m_pScene = nullptr;
    m_pRNG = nullptr;
}

ItemDropCreator* ItemDropCreator::Instance() {
    // If an instance exists
    assert(m_pInstance);

    // Return it
    return m_pInstance;
}

void ItemDropCreator::SetScene(wolf::Scene* p_pScene) {
    m_pScene = p_pScene;
}

// Create a single drop item using a pointer to an existing item (use when you are dropping an item from an inventory)
wolf::GameObject* ItemDropCreator::CreateItemDropFromExistingItem(ItemBase* p_pItem, const glm::vec2& p_v2SpawnPos, float p_fLifespan) {
    // Create the item's gameobject
    wolf::GameObject* pItemDropGO = &m_pScene->CreateObject2D();

    // Add the dropped item component and the sprite
    pItemDropGO->AddComponent<DroppedItemComponent>(p_pItem, p_fLifespan);

    // Add the animated sprite component and rig up the default animation (literally just show the item's sprite forever)
    auto pItemDropAnim = &pItemDropGO->AddComponent<AnimatedSprite2D>(ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), 1.0f);
    pItemDropAnim->AddAnimation("Display", ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), p_pItem->GetTextureFrameIndex(), p_pItem->GetTextureFrameIndex(), glm::vec2(16.0f, 16.0f), false, "");
    pItemDropAnim->SetAnimation("Display");

    // Move the gameobject to the spawn location
    auto pItemDropTransform = pItemDropGO->GetComponent<wolf::Transform2D>();
    pItemDropTransform->SetPosition(p_v2SpawnPos + glm::vec2(m_pRNG->NextFloat(-1.5f, 1.5f), m_pRNG->NextFloat(-1.5f, 1.5f)));

    // Add the collider
    auto& pCollider = pItemDropGO->AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true, m_pScene->GetPlayerGoId());
    pCollider.AddColliderBox(glm::vec2(16.0f, 16.0f), glm::vec2(-8.0f, 8.0f));

    // Add the velocity component
    auto& pVelocity = pItemDropGO->AddComponent<VelocityComponent>();

    // Then return a reference to the gameobject we created
    return pItemDropGO;
}

// Create a single drop item using the .yaml item directory (use when you want to drop a specific item)
wolf::GameObject* ItemDropCreator::CreateItemDropFromDirectory(const std::string& p_strItemName, const glm::vec2& p_v2SpawnPos, float p_fLifespan) {
    // Attempt to create the item requested
    ItemBase* pItem = ItemCreator::CreateItem(p_strItemName);
    
    // If the item was successfully created
    if (pItem) {
        // Create the item's gameobject
        wolf::GameObject* pItemDropGO = &m_pScene->CreateObject2D();

        // Add the dropped item component and the sprite
        pItemDropGO->AddComponent<DroppedItemComponent>(pItem, p_fLifespan);

        // Add the animated sprite component and rig up the default animation (literally just show the item's sprite forever)
        auto pItemDropAnim = &pItemDropGO->AddComponent<AnimatedSprite2D>(ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), 1.0f);
        pItemDropAnim->AddAnimation("Display", ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), pItem->GetTextureFrameIndex(), pItem->GetTextureFrameIndex(), glm::vec2(16.0f, 16.0f), false, "");
        pItemDropAnim->SetAnimation("Display");

        // Move the gameobject to the spawn location
        auto pItemDropTransform = pItemDropGO->GetComponent<wolf::Transform2D>();
        pItemDropTransform->SetPosition(p_v2SpawnPos + glm::vec2(m_pRNG->NextFloat(-1.5f, 1.5f), m_pRNG->NextFloat(-1.5f, 1.5f)));

        // Add the collider
        auto& pCollider = pItemDropGO->AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true, m_pScene->GetPlayerGoId());
        pCollider.AddColliderBox(glm::vec2(16.0f, 16.0f), glm::vec2(-8.0f, 8.0f));

        // Add the velocity component
        auto& pVelocity = pItemDropGO->AddComponent<VelocityComponent>();

        // Then return a reference to the gameobject we created
        return pItemDropGO;
    }
    else { // If we couldn't create the item
        return nullptr;
    }
}

// Create a single drop item using a .yaml loot table (use when you want to drop a non-specific item)
wolf::GameObject* ItemDropCreator::CreateItemDropFromLootTable(const std::string& p_strLootTable, const glm::vec2& p_v2SpawnPos, float p_fLifespan) {
    // Temporary variables to hold the YAML loot table file and the item we create from it
    YAML::Node node;
    ItemBase* pItem;

    // If we've seen this loot table before
    if (m_mLootTables.contains(p_strLootTable)) {
        // Retrieve it from the map
        node = m_mLootTables.at(p_strLootTable);
    }
    else { // If this is a loot table we've never seen before
        // Load the file
        node = YAML::LoadFile(p_strLootTable);

        // And add it to the loot table map
        m_mLootTables.insert({p_strLootTable, node});
    }

    // Parse the loot table YAML file
    // *** This try-catch block was taken directly from D'Anyil's ChestInventoryComponent FillFromLootTable method ***
    try {
        // Get the number of items if it exists
        int numItems = node["items"] ? node["items"].as<int>() : 0;

        // If it doesn't exist, calculate it from min and max
        if (numItems == 0)
        {
            numItems = m_pRNG->NextInt(node["min_items"].as<int>(), node["max_items"].as<int>());
        }

        // Grab the loot table node
        YAML::Node lootTable = node["loot_table"];
        int numEntries = lootTable.size();

        // Initialize an array of entries
        YAML::Node entries[numEntries];

        // Count the number of items in the loot table
        float sumWeights = 0;
        for (int i = 0; i < numEntries; ++i)
        {
            entries[i] = lootTable[i];
            const YAML::Node& entry = entries[i];
            sumWeights += entry["probability"].as<float>();
        }

        for (int i = 0; i < numItems; ++i)
        {
            // Generate a random number
            float value = m_pRNG->NextFloat(0.0f, sumWeights);
            std::string chosenItem;
            for (int j = 0; j < numEntries; ++j)
            {
                float probability = entries[j]["probability"].as<float>();
                
                if (value < probability)
                {
                    chosenItem = entries[j]["name"].as<std::string>();
                    break;
                }
                value -= probability;
            }

            // Create the item
            pItem = ItemCreator::CreateItem(chosenItem);
            if (!pItem)
            {
                wolf::Error("Item name invalid: ", chosenItem);
                continue;
            }
        }
    }
    catch (YAML::Exception& e) // If there was a problem with the loot table YAML file
    {
        // If we run into an error, then we should print it and return false
        wolf::Error("YAML: Issue with ", p_strLootTable.c_str(), ": ", e.what());
        return nullptr;
    }

    // If the item was successfully created
    if (pItem) {
        // Create the item's gameobject
        wolf::GameObject* pItemDropGO = &m_pScene->CreateObject2D();

        // Add the dropped item component and the sprite
        pItemDropGO->AddComponent<DroppedItemComponent>(pItem, p_fLifespan);

        // Add the animated sprite component and rig up the default animation (literally just show the item's sprite forever)
        auto pItemDropAnim = &pItemDropGO->AddComponent<AnimatedSprite2D>(ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), 1.0f);
        pItemDropAnim->AddAnimation("Display", ITEM_TEXTURE_PATH, glm::vec2(32.0f, 32.0f), pItem->GetTextureFrameIndex(), pItem->GetTextureFrameIndex(), glm::vec2(16.0f, 16.0f), false, "");
        pItemDropAnim->SetAnimation("Display");

        // Move the gameobject to the spawn location
        auto pItemDropTransform = pItemDropGO->GetComponent<wolf::Transform2D>();
        pItemDropTransform->SetPosition(p_v2SpawnPos + glm::vec2(m_pRNG->NextFloat(-1.5f, 1.5f), m_pRNG->NextFloat(-1.5f, 1.5f)));

        // Add the collider
        auto& pCollider = pItemDropGO->AddComponent<ColliderComponent>(ColliderComponent::HITBOX, false, true, m_pScene->GetPlayerGoId());
        pCollider.AddColliderBox(glm::vec2(16.0f, 16.0f), glm::vec2(-8.0f, 8.0f));

        // Add the velocity component
        auto& pVelocity = pItemDropGO->AddComponent<VelocityComponent>();

        // Then return a reference to the gameobject we created
        return pItemDropGO;
    }
    else { // If we couldn't create the item
        return nullptr;
    }
}