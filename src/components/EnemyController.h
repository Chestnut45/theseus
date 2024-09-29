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
        DEATH
    };

    // Constructor to set initial values
    EnemyController(float chaseSpeed = 150.0f);

    // Called once when the component is added to a game object
    void Init() ;

    // Update function called every frame
    void Update(float delta) ;

private:
    // Handle different states
    void HandleIdleState(float delta);
    void HandleChasingState(float delta);
    void HandleDeathState();

    // Helper to move towards the target
    void MoveTowardsTarget(float delta);

    // Components
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    HealthComponent* m_pHealth = nullptr;
    AnimatedSprite2D* m_pAnim = nullptr;

    // Current state
    EnemyState m_state = EnemyState::IDLE;

    // Target (e.g., player object)
    wolf::GameObject* m_pTarget = nullptr;

    // Movement speed when chasing
    float m_chaseSpeed;

    // Helper to check if the enemy is in range to chase the player
    bool IsPlayerInRange() const;
};
