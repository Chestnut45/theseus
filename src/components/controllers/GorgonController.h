#pragma once

//-----------------------------------------------------------------------------
// File: GorgonController.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls gorgon attacks & behaviours.
//-----------------------------------------------------------------------------

#include "EnemyController.h"
#include <wolf.h>
#include <VelocityComponent.h>
#include <HealthComponent.h>
#include <ColliderComponent.h>
#include <AnimatedSprite2D.h>
#include <StatusComponent.h>
#include <EnemyDataLoader.h>
#include "InfightingEvent.h"
#include "PathfindingManager.h"
class GorgonController : public EnemyController
{
public:
    GorgonController();    
    ~GorgonController(); 
    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);

    wolf::GameObject* GetTarget() const { return m_pTarget; }
    EnemyState GetState() const { return m_state; }

private:
    // Gorgon-specific methods
    void SetUpAnimations(const std::string& animationInitPath);          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);
    void HandleIdleState(float delta);
    void HandleProspectState(float delta);
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandlePetrifiedState(float delta);
    void HandleStunnedState(float delta);
    void HandleDeathState(float delta);
    
    void EnterAttackState();
    void EnterChasingState();
    void EnterIdleState();
    void EnterPetrifiedState();
    void EnterProspectState();
    void EnterStunnedState();
    void EnterDeathState();

    void ExitAttackState();
    void ExitChasingState();
    void ExitIdleState();
    void ExitPetrifiedState();
    void ExitProspectState();
    void ExitStunnedState();

    void SetEmote(EnemyEmote p_emote);

    bool IsTargetDetected();
    bool IsTargetInLOS(); // Check if target is in line of sight
    void RenderIndicator();

    void HandleInfighting(const InfightingEvent& event); // added this
    void RevertToPlayerTarget();

    
    // Gorgon-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    StatusComponent* m_pTargetStatusComponent = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    float m_meleeRange;
    float m_rangedRange;
    float m_rangedCooldown; // Delay between 2 ranged attacks
    float m_rangedTimer = 0.0f;
    float m_detectionRange;
    float m_baseDamage;
    float m_chaseSpeed;
    wolf::Timer m_transitionTimer;
    float m_transitionDelay = 0.2f; // Example delay for transitioning states

    PathfindingManager* m_pPathfindingManager = nullptr;
    void FallbackToDirectMovement(float delta);


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
    
    // Stunned state members
    float m_stunnedTime = 0.3f;
    float m_stunnedTimer = 0.0f;

    // Death state members
    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;

    // Attack state members
    float m_rangedWindupTime = 1.0f; // Windup Time
    float m_rangedWindupTimer = 0.0f;

    // Emote-related members
    EnemyEmote m_emote = EnemyEmote::NONE; // Current emote
    const float EMOTE_TIME = 1.0f;
    float m_fEmoteTimer = 0.0f;
    wolf::GameObject* m_pEmoteObj = nullptr;

    glm::vec2 m_vLinecheckEndpoint = glm::vec2(0.0f, 0.0f);
    bool m_IsRenderingAttackIndicator = false;

    const glm::vec4 CROSSHAIR_COLOUR = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 m_curentCrosshairColour = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    glm::vec2 m_crosshairOffset = glm::vec2(0.0f, 0.0f);
};