#pragma once

//-----------------------------------------------------------------------------
// File:            ItemDropCreator.h
// Original Author: Aurora Ryder
//
// A singleton class that is used to create GameObject representations
// of inventory items that can be dropped by entities and picked up by the
// player. Note that this class requires a <> instance to create items.
//-----------------------------------------------------------------------------

#include <W_GameObject.h>
#include <W_Scene.h>
#include <W_RNG.h>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>
#include <map>
#include <vector>

#include "ItemCreator.h"

#include <DroppedItemComponent.h>
#include <AnimatedSprite2D.h>
#include <LabyrinthManager.h>

class ItemDropCreator {
    public:
        static void CreateInstance(wolf::Scene* p_pScene, int p_iRNGSeed);
        static void DestroyInstance();
        static ItemDropCreator* Instance();

        void SetScene(wolf::Scene* m_pScene);

        wolf::GameObject* CreateItemDropFromExistingItem(ItemBase* p_pItem, const glm::vec2& p_v2SpawnPos, float p_fLifespan);
        wolf::GameObject* CreateItemDropFromDirectory(const std::string& p_strItemName, const glm::vec2& p_v2SpawnPos, float p_fLifespan);
        std::vector<wolf::GameObject*> CreateItemDropFromLootTable(const std::string& p_strLootTable, const glm::vec2& p_v2SpawnPos, float p_fLifespan);

    private:
        ItemDropCreator() {};
        ~ItemDropCreator() {};

        // Delete copy constructor/assignment
        ItemDropCreator(const ItemDropCreator&) = delete;
        ItemDropCreator& operator=(const ItemDropCreator&) = delete;

        // Delete move constructor/assignment
        ItemDropCreator(ItemDropCreator&& other) = delete;
        ItemDropCreator& operator=(ItemDropCreator&& other) = delete;

        static const std::string ITEM_TEXTURE_PATH;

        static ItemDropCreator* m_pInstance;
        static wolf::Scene* m_pScene;
        static LabyrinthManager* m_pLabyrinthManager;
        static wolf::RNG* m_pRNG;

        static int m_iRNGSeed;

        static std::map<std::string, YAML::Node> m_mLootTables;
};