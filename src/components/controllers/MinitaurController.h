#pragma once
//-----------------------------------------------------------------------------
// File: MinitaurController.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls minitaur attacks & behaviours.
//-----------------------------------------------------------------------------

#include "EnemyController.h"
#include <wolf.h>
#include <VelocityComponent.h>
#include <HealthComponent.h>
#include <ColliderComponent.h>
#include <AnimatedSprite2D.h>
#include <EnemyDataLoader.h>
#include "PathfindingManager.h"
#include <NavMeshComponent.h>
#include "InfightingEvent.h"

class MinitaurController : public EnemyController
{
public:
    MinitaurController();    
    ~MinitaurController();

    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);
    EnemyState GetState() const { return m_state; }
    wolf::GameObject* GetTarget() const { return m_pTarget; }


private:
    // Minitaur-specific methods
    void SetUpAnimations(const std::string& animationInitPath);          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);

    // State handler methods
    void HandleIdleState(float delta);
    void HandleProspectState(float delta);
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandlePetrifiedState(float delta);
    void HandleStunnedState(float delta);
    void HandleDeathState(float delta);

    // State enter methods
    void EnterAttackState();
    void EnterChasingState();
    void EnterIdleState();
    void EnterPetrifiedState();
    void EnterProspectState();
    void EnterStunnedState();
    void EnterDeathState();

    // State exit methods
    void ExitAttackState();
    void ExitChasingState();
    void ExitIdleState();
    void ExitPetrifiedState();
    void ExitProspectState();
    void ExitStunnedState();

    // Helper methods
    void HandleInfighting(const InfightingEvent& event); //added this
    void RevertToPlayerTarget();
    void SetEmote(EnemyEmote p_emote);

    bool IsTargetDetected();
    bool IsTargetInLOS(); // Check if target is in line of sight
    glm::vec2 GetTileWorldPos(glm::ivec2 p_tile_pos);
    void FallbackToDistanceChecking();

    
    // Minitaur-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    float m_meleeRange;
    float m_meleeCooldown;  // Delay between 2 melee attacks
    float m_meleeTimer = 0.0f;
    float m_detectionRange;
    float m_baseDamage;
    float m_chaseSpeed;
    wolf::Timer m_transitionTimer;
    float m_transitionDelay = 0.2f; // Example delay for transitioning states
    std::vector<glm::ivec2> m_path;               // Current path to the player
    size_t m_currentPathIndex = 0;                // Index of the current tile in the path
    PathfindingManager* m_pPathfindingManager = nullptr;  // Pointer to the pathfinding manager
    glm::ivec2 m_lastTargetTile; // Tracks the last target tile
    glm::ivec2 m_lastStartTile;  // Tracks the last start tile
    void RenderDebugPath();
        

    //-----------------//
    //                 //
    //  Added by Nhật  //
    //                 //
    //-----------------//

    wolf::RNG m_RNG; 

    float m_targetDetectionTimer = 0.0f; // Taget detecction reaction delay

    // Prospect state members
    float m_prospectCounter = 0.0f;
    float m_prospectStandingCounter = 2.0f;
    float m_stunnedTime = 0.3f;
    float m_stunnedTimer = 0.0f;
    
    // Death state members
    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;

    // Attack state members
    float m_meleeWindupTime = 1.0f; // Windup Time
    float m_meleeWindupTimer = 0.0f;

    EnemyEmote m_emote = EnemyEmote::NONE; // Current emote
    const float EMOTE_TIME = 1.0f;
    float m_fEmoteTimer = 0.0f;
    wolf::GameObject* m_pEmoteObj = nullptr;

    NavMeshComponent* m_pNavMeshComponent = nullptr;
    std::vector<glm::vec2> m_navMeshPath;
    float m_navMeshPathUpdateTimer = 0.0f;
    glm::vec2 m_lastPosition = glm::vec2(0.0f);
    float m_stuckTimer = 0.0f;
    bool m_useNavMesh = true;

    // SFX timer
    wolf::Timer m_oinkTimer;
    float m_nextOinkTime = 0.25f;
};