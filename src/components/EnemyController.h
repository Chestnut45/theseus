#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/HealthComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/PlayerController.h>

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

    // Constructor to set initial values
    EnemyController(float chaseSpeed = 150.0f);

    // Called once when the component is added to a game object
    void Init() ;

    // Update function called every frame
    void Update(float delta) ;

private:
    // Handle states
    void HandleIdleState(float delta);
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);  // NEW
    void HandleDeathState();

    // Attack functions
    void ApplyDamageToPlayer();  // NEW

    // Utility
    void MoveTowardsTarget(float delta);
    bool IsPlayerInRange() const;

    // Components
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    HealthComponent* m_pHealth = nullptr;
    wolf::GameObject* m_pTarget = nullptr;

    // State and properties
    EnemyState m_state = EnemyState::IDLE;
    float m_chaseSpeed = 100.0f;

    // Attack properties
    float m_meleeRange = 50.0f;         // Melee attack range
    float m_attackCooldown = 1.0f;      // Time between attacks
    float m_attackTimer = 0.0f;         // Tracks time until next attack
    float m_baseDamage = 10.0f;         // Base damage dealt by melee attacks
};
