//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// Modifications : D'Anyil Landry, Nguyễn Minh Nhật, Aurora Ryder
// ver 1.1
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "GameState.h"

#include <Theseus.h>

#include <W_Sprite2D.h>
#include <W_FrameBuffer.h>

#include <ColliderManager.h>

#include <EquipmentItem.h>
#include <FlatAmtItem.h>
#include <PercentItem.h>
#include <StatusEffectItem.h>
#include <ItemCreator.h>
#include <ItemDropCreator.h>

#include <DialogueAndCutsceneEvent.h>
#include <TriggerEvent.h>
#include <GameOverEvent.h>

#include <TrapComponent.h>
#include <EnemyController.h>
#include <MinitaurBuilder.h>
#include <GorgonBuilder.h>
#include <HarpyBuilder.h>
#include <TriggerComponent.h>
#include "EnemyDataLoader.h"
#include "PathfindingManager.h"
#include <events/GameWinEvent.h>
#include <events/LabyrinthEvents.h>
#include <W_Timer.h>
#include <NavMeshComponent.h>
#include <unordered_map>
#include <unordered_set>
#include <ParticleEditor.h>

class LabyrinthManager;

class PlayState : public GameState
{
public:

    PlayState(GameStateManager* manager, Theseus* gameInstance, const std::string& seedText, bool debugAllowed);

    void Enter() override;
    void Exit() override;
    void Pause() override;
    void Resume() override;
    void Update(float delta) override;
    void Render(float delta) override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender(float delta) override;
    void OnDialogueAndCutsceneTriggered(const DialogueAndCutsceneEvent& event);
    void OnTriggerEvent(const TriggerEvent& event);
    void OnGameWinEvent(const GameWinEvent& event);
    
    std::unordered_map<std::string, wolf::GameObjectID>& GetEntityIDs() {return m_entityIDs;}

private:

    wolf::GameObject* m_pPlayerObject = nullptr;

    wolf::GameObject* m_pAriadne = nullptr;

    // Managers
    LabyrinthManager* m_pLabyrinthManager = nullptr;
    ColliderManager* m_pColliderManager = nullptr;
    PathfindingManager* m_pPathfindingManager = nullptr;

    std::string m_seedText;

    // Flags
    bool m_debugHotkeys = false;
    bool m_showLabyrinthManager = false;
    bool m_showInventoryGUI = false;
    bool m_noClip = false;
    bool m_renderDebugColliders = false;

    // Boss data
    glm::vec2 m_bossfightPlayerPos;
    wolf::GameObject* m_pBoss = nullptr;
    wolf::GameObject* m_pBossWalls = nullptr;
    wolf::GameObject* m_pBossTrigger = nullptr;
    std::vector<glm::ivec2> m_bossRoomDoorTiles;
    glm::ivec2 m_bossRoomOrigin;
    glm::ivec2 m_bossRoomSize;

    // Timer for transitioning the camera zoom when the player enters the chamber
    wolf::Timer m_bossZoomTimer;

    // Credits data
    wolf::Timer m_cameraShakeTimer;
    wolf::Timer m_fadeToBlackTimer;
    wolf::Timer m_completionMessageTimer;
    wolf::Timer m_showCreditsTimer;
    wolf::Timer m_returnToMainMenuTimer;
    wolf::Timer m_gameCompletionTime;
    bool m_isExiting = false;

    // Field background rendering resources
    GLuint m_dummyVAO = 0;
    wolf::Program* m_pBackgroundShader = nullptr;
    wolf::Texture* m_pFieldTexture = nullptr;

    // Map data
    std::unordered_map<std::string, wolf::GameObjectID> m_entityIDs;
    std::unordered_set<glm::ivec2> m_visitedChunks; // Track visited chunks
    bool m_isMapExpanded = false;                  // Toggle for expanded map

    // Fog rendering data
    static const int FOG_TEX_SIZE = 1024;
    static const int FOG_TEX_FLAT_LENGTH = FOG_TEX_SIZE * FOG_TEX_SIZE;
    glm::vec4 m_cachedMapPosAndSize{0.0f};
    float m_cachedMapZoom = 0.1f;
    wolf::Program* m_pFogMapShader = nullptr;
    wolf::Program* m_pFogWorldShader = nullptr;
    GLuint m_fogTraversalTex = 0;
    std::vector<uint8_t> m_fogMaskTexels;
    friend void FogCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);
    
    // Navigation data
    NavMeshComponent* m_pNavMeshComponent = nullptr;
    std::vector<wolf::GameObject*> m_navMeshObstacles;

    ParticleEditor* m_pParticleEditor = nullptr;
    wolf::FrameBuffer* m_pFBO = nullptr;

    // Private helper methods
    void ConvertPlayerTileToGold();
    void RegisterClosestMinitaur();

    // Entity creation methods
    void CreatePlayer();
    void SpawnBossObjects();
    void DestroyBossObjects();
    wolf::GameObject& CreateAriadneAndReturn(glm::vec2 playerPosition);

    // Handlers
    void OnGameOverEvent(const GameOverEvent& event);
    void OnRegenerateEvent(const LabyrinthRegenerateEvent& event);
    void OnDestroyEvent(const LabyrinthDestroyEvent& event);

    // Displays the open chest tooltip
    void ShowTooltip(const std::string& text);

    // Extra rendering methods
    void RenderMap();
    void RenderTextCentered(const std::string& text, float size);
    void RenderCredits(float delta);
    void RenderFadeOverlay(float alpha);

    // Helpers
    bool IsWallTile(int tileID);
};