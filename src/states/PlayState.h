//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// Modifications : D'Anyil Landry
// ver 1.1
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once
#include "glm_hash.h"
#include "GameState.h"
#include <theseus.h> // Include the main game class
#include <W_Sprite2D.h>

#include "../inventory/EquipmentItem.h"
#include "../inventory/FlatAmtItem.h"
#include "../inventory/PercentItem.h"
#include "../inventory/StatusEffectItem.h"
#include "../inventory/ItemCreator.h"
#include "../inventory/ItemDropCreator.h"
#include "../events/DialogueAndCutsceneEvent.h"
#include "../ColliderManager.h"
#include "events/TriggerEvent.h"
#include "events/GameOverEvent.h"
#include <TrapComponent.h>
#include <EnemyController.h>
#include <MinitaurBuilder.h>
#include <GorgonBuilder.h>
#include <HarpyBuilder.h>
#include <TriggerComponent.h>
#include "EnemyDataLoader.h"

#include <unordered_map>
#include <unordered_set>


class LabyrinthManager;

class PlayState : public GameState
{
public:
    PlayState(GameStateManager* manager, Theseus* gameInstance)
        : GameState(manager, gameInstance) {}


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
    std::unordered_map<std::string, wolf::GameObjectID>& GetEntityIDs() {return m_entityIDs;}

private:
    
    // Game objects / components that will exist for the duration of the play state
    wolf::GameObject* m_pPlayerObject = nullptr;
    LabyrinthManager* m_pLabyrinthManager = nullptr;

    // Manager for colliders
    ColliderManager* m_pColliderManager = nullptr;

    // Flags
    bool m_showLabyrinthManager = false;
    bool m_showInventoryGUI = false;
    bool m_noClip = false;

    // Location to spawn player when bossfight starts
    glm::vec2 m_bossfightPlayerPos;
    wolf::GameObject* m_pBoss = nullptr;
    wolf::GameObject* m_pBossWalls = nullptr;
    std::vector<glm::ivec2> m_bossRoomDoorTiles;
    glm::ivec2 m_bossRoomOrigin;
    glm::ivec2 m_bossRoomSize;

    // Private helper methods
    void ConvertPlayerTileToGold();

    // Creates the player object and all of its components
    // PRE: The player must not have been created yet
    // POST: m_pPlayerObject will be set to a pointer to the newly created player object
    void CreatePlayer();
    void CreateMinitaurEnemy();
    void CreateHarpyEnemy();
    void CreateGorgonEnemy();
    void CreateTrappedChest();
    wolf::GameObject& CreateAriadneAndReturn(glm::vec2 playerPosition);



    void CreateThrowableObject();
    wolf::GameObject& CreateSpikeTrap(const glm::vec2& position);
    wolf::GameObject& CreateBoulderTrap(const glm::vec2& position);

    void OnGameOverEvent(const GameOverEvent& event);

   // TO DO: New Helper Methods for BoulderTrap

    // Displays the open chest tooltip
    void ShowTooltip(const std::string& text);

    void RenderMap();
    bool IsWallTile(int tileID);

    std::unordered_map<std::string, wolf::GameObjectID> m_entityIDs;

    std::unordered_set<glm::ivec2> m_visitedChunks; // Track visited chunks
    bool m_isMapExpanded = false;                  // Toggle for expanded map
};