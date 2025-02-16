//-----------------------------------------------------------------------------
// File: PortalTileManager.h
// Original Author: Nguyễn Minh Nhật
// Manager for portal tiles.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include <LabyrinthManager.h>


class PortalTileManager
{
public:
    static void CreateInstance(LabyrinthManager* p_lbmg);
    static void DestroyInstance();
    static PortalTileManager* GetInstance();

    void Update(float p_dt);

    void CreatePortalTilePair(glm::ivec2 p_tile_pos_1, glm::ivec2 p_tile_pos_2);

private:
    PortalTileManager(LabyrinthManager* p_lbmg);
    virtual ~PortalTileManager();
    bool IsValidTile(glm::ivec2 p_tile_pos) const;
    

    struct PortalTile
    {
    public:
        static std::pair<PortalTile*, PortalTile*> CreatePair(glm::ivec2 p_tile_pos_1, glm::ivec2 p_tile_pos_2, LabyrinthManager* p_lbmg);
        static void DeletePair(PortalTile* p_protal_tile_1, PortalTile* p_protal_tile_2);
        void Update(float p_dt);
        
        // Getters
        glm::ivec2 GetTilePos() const;
        bool IsActive() const;
        PortalTile* GetSibling() const;
        wolf::GameObjectID GetArrivalID() const;
        wolf::GameObject* GetChunk() const;
        glm::ivec2 GetChunkID() const;

        // Setters
        void SetActive(bool p_active);
        void SetArrivalID(wolf::GameObjectID p_arrival_id);
        
        // Helpers
        void CheckTeleport(wolf::GameObject* p_obj);
        void Teleport(wolf::GameObject* p_obj);

    private:
        glm::ivec2 m_vTilePos = glm::ivec2(0.0f, 0.0f);
        bool m_bIsActive = false;
        PortalTile* m_pSiblingPortalTile = nullptr;
        wolf::GameObject* m_pPortalTileSpriteObj = nullptr;
        wolf::GameObjectID m_arrivalID = -1;             // Any object teleported to this portal tile that is still occupying it
        wolf::GameObject* m_pChunk = nullptr;
        glm::ivec2 m_vChunkID = glm::ivec2(-1.0f, -1.0f);
        LabyrinthManager* m_pLabyrinthManager = nullptr;
        wolf::GameObject* m_pPlayer = nullptr;
        const glm::vec2 SPAWN_OFFSET = glm::vec2(LabyrinthManager::TILE_SIZE * LabyrinthManager::SCALE * 0.5f);

        PortalTile(glm::ivec2 p_tile_pos, LabyrinthManager* p_lbmg);
        virtual ~PortalTile();
    }; 
    
    LabyrinthManager* m_pLBMG = nullptr;
    std::vector<std::pair<PortalTile*, PortalTile*>> m_vPortalTilePairs;

    static PortalTileManager* s_pPTMG;
};