#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/HealthComponent.h>
#include <components/EnemyController.h>
#include <components/HurtboxComponent.h>
#include <components/HitboxComponent.h>
#include <iostream>

//-----------------------------------------------------------------------------
// File:            PlayerController.h
// Original Author: Youssef Ashraf
// ver 1.9. Restructured header for improved readability and state management.
//-----------------------------------------------------------------------------

class PlayerController : public wolf::BaseComponent
{
public:
    // Enum for different player actions
    enum class PlayerAction
    {
        NONE,
        WALKING,
        JUMPING,
        ROLLING,
        ATTACKING
    };

    // Enum for player movement directions
    enum class PlayerDirection
    {
        NONE,
        NORTH,
        NORTH_EAST,
        EAST,
        SOUTH_EAST,
        SOUTH,
        SOUTH_WEST, 
        WEST,
        NORTH_WEST
    };

    // Constructor and initialization methods
    PlayerController();
    void LateInitialize();
    void Update(float delta);
    void Render();
    void SetAnimationComponent(AnimatedSprite2D* animComponent);
    void SetManagers(HitboxManager* pHitboxManager, HurtboxManager* pHurtboxManager);

    // Overloaded << operator for printing directions
    friend std::ostream& operator<<(std::ostream& os, const PlayerController::PlayerDirection& direction);

private:
    // Initialization and animation management
    void InitializeAnimations();
    void SetAnimationBasedOnState();
    void HandleMovement(float delta);
    void HandleRolling(float delta);
    void HandleJumping(float delta);
    void HandleAttacking(float delta);
    void StartAttack();
    void UpdateAttackState(float delta);
    void ApplyDamageToEnemy();
    void RegenerateStamina(float delta);
    std::string GetAttackAnimationForDirection(PlayerDirection direction) const;
    std::vector<int> m_heldKeys;
    void AddHeldKey(int key);
    void RemoveHeldKey(int key);
    PlayerDirection GetDirectionFromHeldKeys() const;

    // Utility methods for direction management
    PlayerDirection GetDirectionFromVector(const glm::vec2& direction) const;
    glm::vec2 GetDirectionVector(PlayerDirection direction) const;
    int m_lastKeyPressed = -1;

    // Components and state variables
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimComponent = nullptr;

    // Animation and movement management
    PlayerAction m_action = PlayerAction::NONE;
    PlayerDirection m_lastDirectionEnum = PlayerDirection::NONE;
    float m_moveSpeed = 200.0f;

    // Stamina and rolling management
    bool m_isRolling = false;
    float m_rollSpeed = 400.0f;
    float m_rollTimer = 0.0f;
    float m_rollDuration = 0.5f;
    float m_stamina = 100.0f;
    const float m_maxStamina = 100.0f;
    const float m_staminaRegenRate = 20.0f;
    const float m_staminaRegenDelay = 1.0f;
    wolf::Timer m_staminaRegenTimer;

    // Jumping management
    bool m_isJumping = false;
    float m_jumpHeight = 10.0f;
    float m_jumpSpeed = 300.0f;
    float m_jumpTimer = 0.0f;

    // Attacking management
    bool m_isAttacking = false;
    float m_attackCooldown = 0.5f;
    float m_attackDamage = 25.0f;
    wolf::Timer m_attackCooldownTimer;
    wolf::Timer m_attackTimer;

    // Collision components
    HitboxManager* m_pHitboxManager = nullptr;
    HurtboxManager* m_pHurtboxManager = nullptr;

    // State management flags
    bool m_animationFinished = false;
};

