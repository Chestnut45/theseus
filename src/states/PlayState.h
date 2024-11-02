//-----------------------------------------------------------------------------
// File:			PlayState.h
// Original Author:	Youssef Ashraf
// Modifications : D'Anyil Landry
// ver 1.1
// A class that's responsible for the Concrete Play State.
//-----------------------------------------------------------------------------
#pragma once
#include "GameState.h"
#include <theseus.h> // Include the main game class
#include <W_Sprite2D.h>

#include "../inventory/EquipmentItem.h"
#include "../inventory/FlatAmtItem.h"
#include "../inventory/PercentItem.h"
#include "../inventory/StatusEffectItem.h"
#include "../inventory/ItemCreator.h"
#include "../events/DialogueTriggerEvent.h"
#include "../ColliderManager.h"
#include "../DialogueManager.h"

#include <TrapComponent.h>
#include <EnemyController.h>
#include <MinitaurBuilder.h>
#include <HarpyBuilder.h>
#include <TriggerComponent.h>
#include "EnemyDataLoader.h"


class LabyrinthManager;

class PlayState : public GameState
{
public:
    PlayState(GameStateManager* manager, Theseus* gameInstance, DialogueManager* dialogueManager)
        : GameState(manager, gameInstance), m_pDialogueManager(dialogueManager) {}


    void Enter() override;
    void Exit() override;
    void Pause() override;
    void Resume() override;
    void Update(float delta) override;
    void Render() override;
    void BackgroundUpdate(float delta) override;
    void BackgroundRender() override;
    void OnDialogueTriggerEvent(const DialogueTriggerEvent& event);

private:
    
    // Game objects / components that will exist for the duration of the play state
    wolf::GameObject* m_pPlayerObject = nullptr;
    LabyrinthManager* m_pLabyrinthManager = nullptr;
    DialogueManager* m_pDialogueManager = nullptr;

    // Manager for colliders
    ColliderManager* m_pColliderManager = nullptr;

    // Flags
    bool m_showLabyrinthManager = false;
    bool m_showInventoryGUI = false;

    // Private helper methods
    void StartDialogue(const std::string& dialogueID);
    // Creates the player object and all of its components
    // PRE: The player must not have been created yet
    // POST: m_pPlayerObject will be set to a pointer to the newly created player object
    void CreatePlayer();
    void CreateMinitaurEnemy();
    void CreateHarpyEnemy();
    void CreatePressurePlate(const glm::vec2& position, TriggerType triggerType, float trapDamage, float trapLifespan, const glm::vec2& trapOffset);
};