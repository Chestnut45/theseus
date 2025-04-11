#pragma once

//-----------------------------------------------------------------------------
// File:            NPCBuilder.h
// Original Author: Aurora Ryder
//
// This singleton lets users build NPCs by either passing in a specific .yaml
// file that describes the character, or by using the scene's RNG to choose and
// build a random NPC from the class' stored directory.
//-----------------------------------------------------------------------------

#include <yaml-cpp/yaml.h>
#include <W_GameObject.h>
#include <W_Logging.h>
#include <W_Scene.h>
#include <W_RNG.h>

#include <HealthComponent.h>
#include <ColliderComponent.h>
#include <AnimatedSprite2D.h>
#include <VelocityComponent.h>

#include <MerchantInventoryComponent.h>
#include <NPCComponent.h>
#include <imgui/imgui.h>

class NPCBuilder {
    public:
        static void CreateInstance(wolf::Scene* p_pScene, int p_iRNGSeed);
        static void DestroyInstance();
        static NPCBuilder* Instance();

        void SetScene(wolf::Scene* p_pScene);

        wolf::GameObject* BuildNPC(const std::string& p_strFilePath);
        wolf::GameObject* BuildRandomNPC();

    private:
        // Private constructor and destructor because this is a singleton
        NPCBuilder() {};
        ~NPCBuilder() {};

        // Delete copy constructor/assignment
        NPCBuilder(const NPCBuilder&) = delete;
        NPCBuilder& operator=(const NPCBuilder&) = delete;

        // Delete move constructor/assignment
        NPCBuilder(NPCBuilder&& other) = delete;
        NPCBuilder& operator=(NPCBuilder&& other) = delete;

        static NPCBuilder* m_pInstance;
        static wolf::Scene* m_pScene;
        static wolf::RNG* m_pRNG;

        static int m_iRNGSeed;

        static const std::string NPC_DIRECTORY_PATH;
        static const ImVec2 NPC_INVENTORY_DRAW_POS;
};