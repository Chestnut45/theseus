#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/PlayerController.h>
#include <components/ColliderComponent.h>

class EnemyController : public wolf::BaseComponent
{
public:
    enum class EnemyState
    {
        IDLE,
        CHASING,
        ATTACKING,
        DEATH
    };

    EnemyController(float chaseSpeed = 150.0f);
    ~EnemyController();

    void Init();
    void Update(float delta);
    void SetColliderManager(ColliderManager* pColliderManager);
    ColliderManager* GetColliderManager() const;


private:
    std::string GetStateAsString(EnemyState state);
    void HandleIdleState();
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandleDeathState();

    void MoveTowardsTarget(float delta);
    void ApplyDamageToPlayer();
    bool IsPlayerInRange() const;

    // Method to set up the animations
    void SetUpAnimations();  
    // Method to update animations based on the direction and state
    void UpdateAnimationBasedOnStateAndDirection();
    

    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    HealthComponent* m_pHealth = nullptr;
    wolf::GameObject* m_pTarget = nullptr;
    AnimatedSprite2D* m_pAnimComponent = nullptr;

    EnemyState m_state = EnemyState::IDLE;
    float m_chaseSpeed = 100.0f;
    float m_meleeRange = 50.0f;
    float m_attackCooldown = 1.0f;
    float m_attackTimer = 0.0f;
    float m_baseDamage = 10.0f;

    glm::vec2 m_currentDirection = glm::vec2(0.0f);  // Current movement direction for smooth transitions

    // Timers for managing state transitions and attack behavior
    wolf::Timer m_transitionTimer;  // Timer for transitioning between states
    float m_transitionDelay = 0.2f; // Delay between state transitions (e.g., 0.2 seconds)

    wolf::Timer m_attackAnticipationTimer;    // Timer for attack anticipation phase
    float m_attackAnticipationDuration = 0.15f; // Duration of anticipation phase before applying damage

    wolf::Timer m_attackRecoveryTimer;      // Timer for recovery phase after attacking
    float m_attackRecoveryDuration = 0.3f;  // Duration of recovery phase after an attack
    ColliderComponent* m_pCollider = nullptr;
    ColliderManager* m_pColliderManager = nullptr;
    
};
