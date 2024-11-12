#include "ItemDropCreator.h"

ItemDropCreator* ItemDropCreator::m_pInstance = nullptr;
wolf::Scene* ItemDropCreator::m_pScene = nullptr;

void ItemDropCreator::CreateInstance(wolf::Scene* p_pScene) {
    // If there is not already an existing instance
    assert(m_pInstance == nullptr);

    // Create one
    m_pInstance = new ItemDropCreator();
    m_pScene = p_pScene;
}

void ItemDropCreator::DestroyInstance() {
    // If an instance exists
    assert(m_pInstance != nullptr);

    // Delete it
    delete(m_pInstance);
    m_pInstance = nullptr;
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

// Create a single drop item using the .yaml item directory (use when you want to drop a specific item)
wolf::GameObject* ItemDropCreator::CreateItemDropFromDirectory(const std::string& p_strItemName, const glm::vec2& p_v2SpawnPos) {
    // Attempt to create the item requested
    ItemBase* pItem = ItemCreator::CreateItem(p_strItemName);
    
    // If the item was successfully created
    if (pItem) {
        // Create the item's gameobject
        wolf::GameObject* pItemDropGO = &m_pScene->CreateObject2D();

        // Add the dropped item component and the sprite
        pItemDropGO->AddComponent<DroppedItemComponent>(pItem);
        // !-- Need to talk to D'Anyil about Sprite2D using frame indices as textures --!
        pItemDropGO->AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");

        // Move the gameobject to the spawn location
        auto pItemDropTransform = pItemDropGO->GetComponent<wolf::Transform2D>();
        pItemDropTransform->SetPosition(p_v2SpawnPos);

        // Then return a reference to the gameobject we created
        return pItemDropGO;
    }
    else { // If we couldn't create the item
        return nullptr;
    }
}

// Create a single drop item using a .yaml loot table (use when you want to drop a non-specific item)
wolf::GameObject* ItemDropCreator::CreateItemDropFromLootTable(const std::string& p_strLootTable, const glm::vec2& p_v3SpawnPos) {
    // !-- Need to talk to D'Anyil about making his loot table parser part of ItemCreator --!
    return nullptr;
}