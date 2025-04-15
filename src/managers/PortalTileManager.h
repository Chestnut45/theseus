//-----------------------------------------------------------------------------
// File: PortalTileManager.h
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>

#include <LabyrinthManager.h>
#include <PlaceableItem.h>

class PortalTileManager
{
friend class PortalTile;

public:
    static void CreateInstance(LabyrinthManager* p_lbmg);
    static void DestroyInstance();
    static PortalTileManager* GetInstance();

    void Update(float p_dt);

    bool CreatePortalTile(glm::ivec2 p_tile_pos);
    bool IsTileOccupiedByAnotherPortalTile(glm::ivec2 p_tile_pos);

private:
    struct PortalTile
    {
    public:
        static PortalTile* CreatePortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg, PortalTile* p_sibling = nullptr);
        static void DeletePortalAndSibling(PortalTile* p_protal_tile_1);
        static void DeleteAvailablePortalTile(PortalTile* p_protal_tile);
        
        // Getters
        glm::ivec2 GetTilePos() const;
        PortalTile* GetSibling() const;
        wolf::GameObjectID GetOccupantID() const;
        wolf::GameObject* GetChunk() const;
        glm::ivec2 GetChunkID() const;

        // Setters
        void SetOccupantID(wolf::GameObjectID p_occupant_id);
        
        // Helpers
        void Update(float p_dt);
        void CheckTeleport(wolf::GameObject* p_obj);
        void Teleport(wolf::GameObject* p_obj);

    private:
        glm::ivec2 m_vTilePos = glm::ivec2(0.0f, 0.0f);
        PortalTile* m_pSiblingPortalTile = nullptr;
        wolf::GameObject* m_pPortalTileSpriteObj = nullptr;
        wolf::GameObjectID m_occupantID = -1;             // Any object teleported to this portal tile that is still occupying it
        wolf::GameObject* m_pChunk = nullptr;
        glm::ivec2 m_vChunkID = glm::ivec2(-1.0f, -1.0f);
        LabyrinthManager* m_pLabyrinthManager = nullptr;
        wolf::GameObject* m_pPlayer = nullptr;
        const glm::vec2 SPAWN_OFFSET = glm::vec2(LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE * 0.5f);
        const float EMISSION_CHANCE = 0.005f;
        const float SCALED_TILE_SIZE = LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE;
        static wolf::RNG s_rng; 

        PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg);
        virtual ~PortalTile();
    }; 

    PortalTileManager(LabyrinthManager* p_lbmg);
    virtual ~PortalTileManager();
    void HandleDestroyPlaceableEvent(const DestroyPlaceableEvent& p_event);
    
    static bool IsValidTile(glm::ivec2 p_tile_pos);
    static void RenderCollectPrompt();
    
    LabyrinthManager* m_pLBMG = nullptr;
    PortalTile* m_pAvailablePortalTile = nullptr;
    std::vector<PortalTile*> m_vPortalTiles;
    wolf::GameObject* m_pPlayer = nullptr;
    static PortalTileManager* s_pPTMG;

    int m_iRemovalIndex = -2; // -1 is for the available portal tile
};