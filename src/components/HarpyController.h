#pragma once

#include "EnemyController.h"
#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>
#include <components/AnimatedSprite2D.h>
#include <EnemyDataLoader.h>

class HarpyController : public EnemyController
{
public:
    HarpyController();    
    ~HarpyController(); 
    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);

private:
    // Minitaur-specific methods
    void SetUpAnimations(const std::string& animationInitPath);          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);
    void HandleIdleState(float delta);
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandlePetrifiedState(float delta);
    void HandleStunnedState(float delta);
    void HandleDeathState(float delta);

    void EnterAttackState();
    void EnterChasingState();
    void EnterIdleState();
    void EnterStunnedState();
    void EnterDeathState();

    void ExitAttackState();
    void ExitChasingState();
    void ExitIdleState();
    void ExitPetrifiedState();
    void ExitStunnedState();
    
    void SetEmote(EnemyEmote p_emote);

    // Minitaur-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
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
    float m_rangedCooldown;
    float m_rangedTimer = 0.0f;
    float m_rangedRange = 1.0f;
    float m_stunnedTime = 0.5f;
    float m_stunnedTimer = 0.0f;

    // Death state members
    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;

    // Attack state members
    const float ATTACK_WINDUP_TIME = 0.1f;
    float m_attackWindupTimer = 0.0f;
    bool m_isEnterAttackWindup = false;
    const float ATTACK_STRIKE_TIME = 0.1f;
    float m_attackStrikeTimer = 0.0f;
    bool m_isEnterAttackStrike = false;
    int m_attackChain = 0;

    // Emote-related members
    EnemyEmote m_emote = EnemyEmote::NONE; // Current emote
    const float EMOTE_TIME = 1.0f;
    float m_fEmoteTimer = 0.0f;
    wolf::GameObject* m_pEmoteObj = nullptr;
};
