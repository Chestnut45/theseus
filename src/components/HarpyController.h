#pragma once
//-----------------------------------------------------------------------------
// File: HarpyController.h
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls harpy attacks & behaviours.
//-----------------------------------------------------------------------------

#include "EnemyController.h"
#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>
#include <components/AnimatedSprite2D.h>
#include <EnemyDataLoader.h>
#include "events/InfightingEvent.h"
#include "BehaviorTree.h"
#include "Blackboard.h"

class HarpyController : public EnemyController
{
public:
    HarpyController();    
    ~HarpyController(); 
    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);
    wolf::GameObject* GetTarget() const { return m_pTarget; }

private:
    // Harpy-specific methods
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
    void EnterPetrifiedState();
    void EnterIdleState();
    void EnterStunnedState();
    void EnterDeathState();

    void ExitAttackState();
    void ExitChasingState();
    void ExitIdleState();
    void ExitPetrifiedState();
    void ExitStunnedState();

    void RevertBackToPlayer();
    void HandleInfighting(const InfightingEvent& event); //added this

    void SetEmote(EnemyEmote p_emote);
    bool IsTargetInSight();
    float GetDistanceToTarget() const;
    
    // New behavior tree methods
    void SetupCombatBehaviorTree();
    void EvaluateStrategy();
    void UpdateBlackboard();
    
    // Attack pattern methods
    void PerformSingleShot();
    void PerformSpreadShot();
    void PerformBurstAttack();
    
    // Positioning methods
    bool ShouldReposition();
    glm::vec2 GetOptimalAttackPosition();
    void MoveToOptimalPosition(float delta);
    bool IsPositionSafe(const glm::vec2& position);

    // Harpy-specific properties
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
    float m_rangedCooldown;     // Delay between 2 ranged attacks
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
    float m_rangedWindupTime = 1.0f; // Windup Time
    float m_rangedWindupTimer = 0.0f;
    int m_attackChain = 0;

    // Emote-related members
    EnemyEmote m_emote = EnemyEmote::NONE; // Current emote
    const float EMOTE_TIME = 1.0f;
    float m_fEmoteTimer = 0.0f;
    wolf::GameObject* m_pEmoteObj = nullptr;
    
    // Behavior tree for combat decisions
    std::unique_ptr<BehaviorTree> m_combatBehaviorTree;
    
    // Behavior states and coordination
    enum class AttackPattern { SINGLE, SPREAD, BURST };
    AttackPattern m_currentAttackPattern = AttackPattern::SINGLE;
    float m_repositionTimer = 0.0f;
    float m_repositionDelay = 4.0f; // Time between position evaluations
    bool m_isRepositioning = false;
    glm::vec2 m_targetPosition; // Position to move to when repositioning

    glm::vec2 m_smoothedVelocity = glm::vec2(0.0f);
    const float ANIMATION_SMOOTHING_FACTOR = 0.2f;

};