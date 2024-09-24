#pragma once

//-----------------------------------------------------------------------------
// File:            PlayerController.h
// Original Author: Youssef Ashraf
// ver 1.7. Update constructor to remove need for dependency injection.
//-----------------------------------------------------------------------------

#include <wolf.h>
#include <components/VelocityComponent.h>
#include <components/AnimatedSprite2D.h>

class PlayerController : public wolf::BaseComponent
{
public:
    enum class PlayerAction
    {
        NONE,
        WALKING,
        JUMPING,
        ROLLING
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

private:
    
    // Handle movement input (WASD)
    void HandleMovement(float delta);

    // Handle rolling input (Spacebar)
    void HandleRolling(float delta);

    // Handle jumping input (J key)
    void HandleJumping(float delta);

    
    PlayerDirection GetRollDirection() const;

    // Convert PlayerDirection to a glm::vec2 
    glm::vec2 GetDirectionVector(PlayerDirection direction) const;

    // Pointer to transform and velocity
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;
    AnimatedSprite2D* m_pAnim = nullptr;

    // Player action
    PlayerAction m_action = PlayerAction::NONE;

    // Movement variables
    float m_moveSpeed = 200.0f;

    // Rolling variables
    bool m_isRolling = false;
    float m_rollSpeed = 400.0f;
    float m_rollTimer = 0.0f;
    float m_rollDuration = 0.5f; // Duration of the roll

    // Stamina variables
    float m_stamina = 100.0f;             
    const float m_maxStamina = 100.0f;    
    const float m_staminaRegenRate = 20.0f;  
    const float m_staminaRegenDelay = 1.0f;  
    wolf::Timer m_staminaRegenTimer;

    // Jumping variables
    bool m_isJumping = false;
    float m_jumpHeight = 10.0f;
    float m_jumpSpeed = 300.0f;
    float m_jumpTimer = 0.0f;

    
    glm::vec2 m_lastDirection = glm::vec2(0.0f);
    PlayerDirection m_lastDirectionEnum = PlayerDirection::NONE;
};
