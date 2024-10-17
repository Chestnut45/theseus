#pragma once

#include "EnemyController.h"
#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/ColliderComponent.h>
#include <components/AnimatedSprite2D.h>

class MinitaurController : public EnemyController
{
public:
    MinitaurController();
    virtual ~MinitaurController();

    // Override Init and Update to implement specific behavior for Minitaur
    void Init() override;
    void Update(float delta) override;

private:
    // Minitaur-specific methods
    void SetUpAnimations();          
    void UpdateAnimationBasedOnDirection();
    void MoveTowardsTarget(float delta);
    void HandleIdleState();
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandleDeathState();
    // Minitaur-specific properties
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    VelocityComponent* m_pVelocity = nullptr;;
    float m_meleeRange = 50.0f;
    float m_attackCooldown = 1.0f;
    float m_attackTimer = 0.0f;
    float m_detectionRange = 300.0f;
    float m_baseDamage = 15.0f;      // Minitaur-specific damage value
    float m_chaseSpeed = 100.0f;     // Define m_chaseSpeed here for movement speed
    wolf::Timer m_transitionTimer;
    float m_transitionDelay = 0.2f; // Example delay for transitioning states
};
