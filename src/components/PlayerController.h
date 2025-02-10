#pragma once

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/AnimatedSprite2D.h>
#include <components/HealthComponent.h>
#include <components/EnemyController.h>
#include <components/ColliderComponent.h>
#include <components/InventoryComponent.h>
#include <components/PlayerInventoryComponent.h>
#include <components/ThrowableObjectComponent.h>
#include <iostream>

// !-- Aurora added this --!
#include "../inventory/WeaponItem.h"
#include "../inventory/ArmourItem.h"

// !-- Death Screen Handling --!
#include "../events/GameOverEvent.h"

#include <DamageEvent.h>

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
        ATTACKING,
        IN_INVENTORY,
        PICKING_UP,
        THROWING,
        PETRIFIED,
        DEAD
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
    void Render(float delta);
    
    void SetAnimationComponent(AnimatedSprite2D* animComponent);

    // Overloaded << operator for printing directions
    friend std::ostream& operator<<(std::ostream& os, const PlayerController::PlayerDirection& direction);
    //set collidermanager
    void SetColliderManager(ColliderManager* pColliderManager);
    //get the collider manager (verification)
    ColliderManager* GetColliderManager() const;
    //get player action
    PlayerAction GetPlayerAction() const;

    // Gets the player's currently held weapon item, or nullptr if empty
    WeaponItem* GetHeldWeapon() const { return m_pCurrentWeapon; }

    //set player action
    void SetAction(PlayerAction action);

    // setting the holding object bool variable
    void SetHoldingObject(bool isHolding);
 
    // This getter simply returns the current value of m_lastFaceDirectionEnum, allowing ThrowableObjectComponent to access it.
    PlayerDirection GetLastFacingDirection() const { return m_lastFaceDirectionEnum; }
    glm::vec2 GetLastFacingDirectionVector() const;

private:
    // Initialization and animation management
    void InitializeAnimations();
    void SetAnimationBasedOnState();

    void HandlePlayerInput(float delta); // Declaration for the missing function
    void HandleMovement(float delta);    // Declaration for HandleMovement
    void HandleRolling(float delta);     // Declaration for HandleRolling
    void HandleJumping(float delta);     // Declaration for HandleJumping
    void HandleAttacking(float delta);   // Declaration for HandleAttacking
    void HandleThrowing(float delta);  // Method to handle throwing
    void HandlePetrified(float delta);  // Method to handle being petrified
    void HandleDeath(float delta);  // New method to handle the existential fear of death

    void HandleBowAttack(float delta);
    void HandleSpearAttack(float delta);
    void HandleSwordAttack(float delta);
    void HandleAttackAnimation();
    void HandleBowAttackAnimation();
    void HandleBowRangeIndicator(float delta);
    void CalculateAttackDirection();
    void RenderBowPowerBar();

    glm::vec2 ClampDirection(const glm::vec2& direction) const;

    // !-- Aurora added this --!
    void HandleWeaponEquippedEvent(const WeaponEquippedEvent& p_event);
    void HandleWeaponUnequippedEvent(const WeaponUnequippedEvent& p_event);
    void HandleArmourEquippedEvent(const ArmourEquippedEvent& p_event);
    void HandleArmourUnequippedEvent(const ArmourUnequippedEvent& p_event);

    void OnDamageEvent(const DamageEvent& event);

    // Manage and transition different player states
    void StartAttack();
    void StartPetrified();
    void StartJump();       // Starts a jumping action
    void StartRoll();       // Starts a rolling action
    void StartDeath();
    
    void EndAttacking();
    void EndJump();         // Ends a jumping action
    void EndPetrified();
    void EndRoll();         // Ends a rolling action
    
    void ThrowHeldObject();
    void PickUpObject();
    void DropObject();

    // Utility functions
    void RegenerateStamina(float delta); // Regenerates stamina over time
    void CheckHealth();
    void RenderThrowPowerBar(); // rendering for the power bar
    void RenderDeathScreen();
    void ResetDeathScreenState(); // cool function to reset vars



    // Animation utility functions
    std::string GetAttackAnimationForDirection(PlayerDirection direction) const;
    std::string GetWalkAnimationForDirection(PlayerDirection direction) const;  // Add this declaration
    std::string GetIdleAnimationForDirection(PlayerDirection direction) const;  // Add this declaration
    PlayerDirection GetDirectionFromVector(const glm::vec2& direction) const;
    glm::vec2 GetVectorFromDirection(PlayerDirection direction) const;

    // Input tracking
    // Data members for components and state management
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnimComponent = nullptr;
    ThrowableObjectComponent* m_pHeldObject = nullptr;
    ColliderComponent* m_pCollider = nullptr;

    // Movement and animation state
    PlayerAction m_action = PlayerAction::NONE;
    PlayerAction m_lastAction = PlayerAction::NONE;
    PlayerDirection m_lastMoveDirectionEnum = PlayerDirection::SOUTH;
    PlayerDirection m_lastFaceDirectionEnum = PlayerDirection::SOUTH;
    std::vector<int> m_heldKeys;  // List of currently held keys
    float m_currentMoveSpeed = 200.0f;
    float m_normalMoveSpeed = 200.0f;
    float m_inventoryMoveSpeed = 100.0f;

    // Sound effect properties
    wolf::Timer m_walkSoundTimer;
    float m_walkSoundInterval = 0.333333333f;

    // DEBUG: Godmode flags
    bool m_godmode = false;
    bool m_superSpeed = false;

    // Stamina management
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
    float m_attackCooldown = 0.5f;
    float m_attackDamage = 50.0f;
    float m_attackRange = 100.0f;
    glm::vec2 m_attackDir = glm::vec2(0.0f, 0.0f);
    wolf::Timer m_attackCooldownTimer;
    wolf::Timer m_attackTimer;

    // Bow attack members
    float m_bowChargeScale = 0.0f;          // Current charge of the boe
    float m_bowMaxChargeScale = 1.0f;       // Limit of the power of the charge
    float m_bowChargeRate = 1.0f;           // Multiplier of delta for charge scale
    float m_arrowRange = 0.0f;              // Current range of the arrow given the current charge
    float m_arrowMaxRange = 600.0f;         // Max range
    const int BOW_HOLD_FRAME_COLUMN = 4;    // The column in the bow attack sprite sheet where Theseus stretches his bow the furthest
    const int BOW_ATTACK_FRAMES = 6;
    bool m_bIsChargingOver = false;        // Prevents double-charging by clicking again after releasing mouse
    bool m_currentBowAnim = 0;
    glm::vec4 m_bowRangeIndicatorColour = glm::vec4(0.0f, 1.0f, 0.4f, 1.0f);
    const ImVec2 BOW_POWER_BAR_SIZE = ImVec2(100.0f, 15.0f);

    // Invulnerability after taking damage
    float m_prevHealthFraction = 1.0f;
    float m_invulnSeconds = 1.0f;
    wolf::Timer m_invulnTimer;

    //picking up management
    bool m_isHoldingObject = false;
    float m_chargeTime = 0.0f;  // New variable to store charge time for throws
    float m_throwSpeed = 300.0f;  // Speed multiplier for the throw
    float m_throwPower = 0.0f;       // Power for the throw
    const float m_maxThrowPower = 100.0f; // Max limit for the throw power
    const float m_powerChargeRate = 25.0f; // Rate at which power increases
    const ImVec2 THROW_POWER_BAR_SIZE = ImVec2(100.0f, 15.0f);

    // Animation and state tracking flags
    std::string m_currentAnimation;
    PlayerAction m_previousAction = PlayerAction::NONE;
    PlayerDirection m_previousDirection = PlayerDirection::NONE;

    bool m_inventoryOpen = false;
    bool m_inventoryHovered = false;

    ColliderManager* m_pColliderManager = nullptr;

    // Equipped weapon (no default)
    WeaponItem* m_pCurrentWeapon = nullptr;

    // Death screen related variables
    wolf::Timer m_runtimeTimer;
    double m_deathRuntime = 0.0; // Store the runtime once when player dies
    wolf::Texture* m_deathScreenTexture = nullptr; //death screen texture

    float m_fallDeadTimer = 0.0f;
    float m_lieDeadTimer = 0.0f;
    float m_timeToFallDead = 0.6f;
    float m_timeToLieDead = 0.8f;
    
    // fade transitions
    float m_fadeOpacity = 0.0f; 
    bool m_fadeComplete = false;
    bool m_blackBackgroundLoaded = false;
    float m_messageOpacity = 0.0f;
    bool m_messageFadeComplete = false;
    float m_runtimeOpacity = 0.0f;
    bool m_runtimeFadeComplete = false;
    float m_optionsOpacity = 0.0f;
};

