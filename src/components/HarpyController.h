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
    HarpyController() = default;    
    ~HarpyController() = default; 
    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    void ChangeState(EnemyState newState);

private:
    // Minitaur-specific methods
    void SetUpAnimations(const std::string& animationInitPath);          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);
    void HandleIdleState();
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandlePetrifiedState(float delta);
    void HandleStunnedState(float delta);
    void HandleDeathState(float delta);
    
    // Minitaur-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    float m_meleeRange;
    float m_attackCooldown;
    float m_attackTimer = 0.0f;
    float m_detectionRange;
    float m_baseDamage;
    float m_chaseSpeed;
    wolf::Timer m_transitionTimer;
    float m_transitionDelay = 0.2f; // Example delay for transitioning states

    // Added by Nhật
    wolf::RNG m_RNG;
    float m_rangedRange = 300.0f;
    float m_stunnedTime = 0.5f;
    float m_stunnedTimer = 0.0f;

    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;
};
