#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/HealthComponent.h>
#include <components/EnemyController.h>
#include <components/ColliderComponent.h>
#include <components/InventoryComponent.h>
#include <iostream>

// !-- Aurora added this --!
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.h
// Original Author: Youssef Ashraf
// ver 2.0. Updated to remove deprecated hitbox and hurtbox components.
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
    ~PlayerController();

    // Delete copy constructor/assignment
    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;

    // Delete move constructor/assignment
    PlayerController(PlayerController&& other) = delete;
    PlayerController& operator=(PlayerController&& other) = delete;

    void LateInitialize();
    void Update(float delta);
    void Render();
    void SetAnimationComponent(AnimatedSprite2D* animComponent);

    // Overloaded << operator for printing directions
    friend std::ostream& operator<<(std::ostream& os, const PlayerController::PlayerDirection& direction);
    void SetColliderManager(ColliderManager* pColliderManager);
    ColliderManager* GetColliderManager() const;

    // Weapon & Attack functions

private:
    // Initialization and animation management
    void InitializeAnimations();
    void SetAnimationBasedOnState();

    void HandlePlayerInput(float delta); // Declaration for the missing function
    void HandleMovement(float delta);    // Declaration for HandleMovement
    void HandleRolling(float delta);     // Declaration for HandleRolling
    void HandleJumping(float delta);     // Declaration for HandleJumping
    void HandleAttacking(float delta);   // Declaration for HandleAttacking

    // !-- Aurora added this --!
    void HandleWeaponEquippedEvent(const WeaponEquippedEvent& p_event);
    void HandleArmourEquippedEvent(const ArmourEquippedEvent& p_event);

    // Manage and transition different player states
    void StartAttack();
    void UpdateAttackState(float delta);
    void StartRoll();       // Starts a rolling action
    void EndRoll();         // Ends a rolling action
    void StartJump();       // Starts a jumping action
    void EndJump();         // Ends a jumping action

    // Utility functions
    void ApplyDamageToEnemy(); // Applies damage to enemies in range
    void RegenerateStamina(float delta); // Regenerates stamina over time

    // Animation utility functions
    std::string GetAttackAnimationForDirection(PlayerDirection direction) const;
    std::string GetWalkAnimationForDirection(PlayerDirection direction) const;  // Add this declaration
    std::string GetIdleAnimationForDirection(PlayerDirection direction) const;  // Add this declaration
    PlayerDirection GetDirectionFromVector(const glm::vec2& direction) const;

    // Input tracking
    // Data members for components and state management
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimComponent = nullptr;

    // Movement and animation state
    PlayerAction m_action = PlayerAction::NONE;
    PlayerDirection m_lastDirectionEnum = PlayerDirection::SOUTH;
    std::vector<int> m_heldKeys;  // List of currently held keys
    float m_moveSpeed = 200.0f;

    // Sound effect properties
    wolf::Timer m_walkSoundTimer;
    float m_walkSoundInterval = 0.34f;

    // Stamina management
    bool m_isRolling = false;
    float m_rollSpeed = 400.0f;
    float m_rollTimer = 0.0f;
    float m_rollDuration = 0.5f;
    float m_stamina = 100.0f;
    const float m_maxStamina = 100.0f;
    const float m_staminaRegenRate = 20.0f;
    const float m_staminaRegenDelay = 0.5f;
    wolf::Timer m_staminaRegenTimer;

    // Jumping management
    bool m_isJumping = false;
    float m_jumpHeight = 10.0f;
    float m_jumpSpeed = 300.0f;
    float m_jumpTimer = 0.0f;

    // Attacking management
    bool m_hasAppliedDamage = false;
    bool m_isAttacking = false;
    float m_attackCooldown = 0.5f;
    float m_attackDamage = 50.0f;
    float m_attackRange = 100.0f;
    wolf::Timer m_attackCooldownTimer;
    wolf::Timer m_attackTimer;

    static float s_aAttackCooldown[WeaponType::FISTS + 1];

    // Animation and state tracking flags
    bool m_animationFinished = false;
    std::string m_currentAnimation;
    PlayerAction m_previousAction = PlayerAction::NONE;
    PlayerDirection m_previousDirection = PlayerDirection::NONE;

    ColliderManager* m_pColliderManager = nullptr;

    // Weapons
    WeaponType m_eCurrentWeapon = WeaponType::SWORD;
};