//-----------------------------------------------------------------------------
// File: PortalTileManager.h
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include <LabyrinthManager.h>
#include "inventory/PlaceableItem.h"

class PortalTileManager
{
friend class PortalTile;

public:
    static void CreateInstance(LabyrinthManager* p_lbmg);
    static void DestroyInstance();
    static PortalTileManager* GetInstance();

    void Update(float p_dt);

    bool CreatePortalTile(glm::ivec2 p_tile_pos);

private:
    PortalTileManager(LabyrinthManager* p_lbmg);
    virtual ~PortalTileManager();
    void HandleDestroyPlaceableEvent(const DestroyPlaceableEvent& p_event);
    static bool IsValidTile(glm::ivec2 p_tile_pos);
    static void RenderCollectPrompt();

    struct PortalTile
    {
    public:
        static PortalTile* CreatePortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg, PortalTile* p_sibling = nullptr);
        static void DeletePair(PortalTile* p_protal_tile_1, PortalTile* p_protal_tile_2);
        static void DeleteAvailablePortalTile(PortalTile* p_protal_tile);
        
        // Getters
        glm::ivec2 GetTilePos() const;
        bool IsActive() const;
        PortalTile* GetSibling() const;
        wolf::GameObjectID GetOccupantID() const;
        wolf::GameObject* GetChunk() const;
        glm::ivec2 GetChunkID() const;

        // Setters
        void SetActive(bool p_active);
        void SetOccupantID(wolf::GameObjectID p_occupant_id);
        
        // Helpers
        void Update(float p_dt);
        void CheckTeleport(wolf::GameObject* p_obj);
        void Teleport(wolf::GameObject* p_obj);
        void CheckPlayerCollection();

    private:
        glm::ivec2 m_vTilePos = glm::ivec2(0.0f, 0.0f);
        bool m_bIsActive = false;
        PortalTile* m_pSiblingPortalTile = nullptr;
        wolf::GameObject* m_pPortalTileSpriteObj = nullptr;
        wolf::GameObjectID m_occupantID = -1;             // Any object teleported to this portal tile that is still occupying it
        wolf::GameObject* m_pChunk = nullptr;
        glm::ivec2 m_vChunkID = glm::ivec2(-1.0f, -1.0f);
        LabyrinthManager* m_pLabyrinthManager = nullptr;
        wolf::GameObject* m_pPlayer = nullptr;
        const glm::vec2 SPAWN_OFFSET = glm::vec2(LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE * 0.5f);
        int m_iPairIndex = -1;  // The index of a portal pair in m_vPortalTilePairs

        PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg);
        virtual ~PortalTile();
    }; 
    
    LabyrinthManager* m_pLBMG = nullptr;
    PortalTile* m_pAvailablePortalTile = nullptr;
    std::vector<std::pair<PortalTile*, PortalTile*>> m_vPortalTilePairs;
    wolf::GameObject* m_pPlayer = nullptr;
    static PortalTileManager* s_pPTMG;
};