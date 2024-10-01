#pragma once

//-----------------------------------------------------------------------------
// File:            PlayerController.h
// Original Author: Youssef Ashraf
// ver 1.7. Update constructor to remove need for dependency injection.
//-----------------------------------------------------------------------------

#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/HealthComponent.h>
#include <components/EnemyController.h>
#include <components/HurtboxComponent.h>
#include <components/HitboxComponent.h>

class PlayerController : public wolf::BaseComponent
{
public:
    enum class PlayerAction
    {
        NONE,
        WALKING,
        JUMPING,
        ROLLING,
        ATTACKING
    };

    // Enum for player direction
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

    // Create a player controller component
    PlayerController();

    // Updates the player controller, adjusting transform and velocity if they exist
    void Update(float delta);
    void Render();
    void SetAnimationComponent(AnimatedSprite2D* animComponent);
    void LateInitialize();
    void SetManagers(HitboxManager* pHitboxManager, HurtboxManager* pHurtboxManager);

private:
    void InitializeAnimations();
    void HandleMovement(float delta);
    void HandleRolling(float delta);
    void HandleJumping(float delta);
    void HandleAttacking(float delta);
    void ApplyDamageToEnemy();

    PlayerDirection GetRollDirection() const;
    PlayerDirection GetDirectionFromVector(const glm::vec2& direction) const;
    glm::vec2 GetDirectionVector(PlayerDirection direction) const;

    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimComponent = nullptr;  // Single animation component for all animations

    PlayerAction m_action = PlayerAction::NONE;
    float m_moveSpeed = 200.0f;
    bool m_isRolling = false;
    float m_rollSpeed = 400.0f;
    float m_rollTimer = 0.0f;
    float m_rollDuration = 0.5f; // Duration of the roll
    float m_stamina = 100.0f;             
    const float m_maxStamina = 100.0f;    
    const float m_staminaRegenRate = 20.0f;
    const float m_staminaRegenDelay = 1.0f;  
    wolf::Timer m_staminaRegenTimer;
    bool m_isJumping = false;
    float m_jumpHeight = 10.0f;
    float m_jumpSpeed = 300.0f;
    float m_jumpTimer = 0.0f;
    float m_attackCooldown = 0.5f; // Cooldown between attacks
    float m_attackDamage = 25.0f;
    bool m_isAttacking = false;

    glm::vec2 m_lastDirection = glm::vec2(0.0f);
    PlayerDirection m_lastDirectionEnum = PlayerDirection::NONE;
    wolf::Timer m_attackCooldownTimer;
    wolf::Timer m_attackTimer;
    wolf::Timer m_animationTransitionTimer;
    
    HitboxManager* m_pHitboxManager = nullptr;
    HurtboxManager* m_pHurtboxManager = nullptr;
};