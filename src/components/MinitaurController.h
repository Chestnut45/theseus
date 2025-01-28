#pragma once

#include "EnemyController.h"
#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>
#include <components/AnimatedSprite2D.h>
#include <EnemyDataLoader.h>

class MinitaurController : public EnemyController
{
public:
    MinitaurController();    
    ~MinitaurController();

    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);

private:
    // Minitaur-specific methods
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
    glm::vec2 GetTileWorldPos(glm::ivec2 p_tile_pos);
    
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
};