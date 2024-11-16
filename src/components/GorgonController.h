
#pragma once

#include "EnemyController.h"
#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/StatusComponent.h>
#include <EnemyDataLoader.h>

class GorgonController : public EnemyController
{
public:
    GorgonController() = default;    
    ~GorgonController() = default; 
    void Init(const EnemyData& data); // Pass the data to initialize the controller
    void Update(float delta) override;
    

private:
    // Gorgon-specific methods
    void SetUpAnimations(const std::string& animationInitPath);          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);
    void HandleIdleState();
    void HandleProspectState(float delta);
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandleDeathState(float delta);
    void ChangeState(EnemyState newState) ;
    
    
    // Gorgon-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    StatusComponent * m_pTargetStatusComponent = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    float m_meleeRange;
    float m_rangedRange;
    float m_rangedCooldown;
    float m_rangedTimer = 0.0f;
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

    float m_prospectCounter = 0.0f;
    float m_prospectStandingCounter = 2.0f;
    
    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;

    float m_targetDist = 0.0f;
};
