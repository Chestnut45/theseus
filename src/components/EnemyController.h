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

    EnemyController(float chaseSpeed = 150.0f);

    void Init();
    void Update(float delta);

private:
    void HandleIdleState();
    void HandleChasingState(float delta);
    void HandleAttackingState(float delta);
    void HandleDeathState();

    void MoveTowardsTarget(float delta);
    void ApplyDamageToPlayer();
    bool IsPlayerInRange() const;

    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    HealthComponent* m_pHealth = nullptr;
    wolf::GameObject* m_pTarget = nullptr;

    EnemyState m_state = EnemyState::IDLE;
    float m_chaseSpeed = 100.0f;
    float m_meleeRange = 50.0f;
    float m_attackCooldown = 1.0f;
    float m_attackTimer = 0.0f;
    float m_baseDamage = 10.0f;
};
