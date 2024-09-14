#pragma once
//-----------------------------------------------------------------------------
// File:            PlayerController.h
// Original Author: Youssef Ashraf
// ver 1.1, added VelocityComponent support for movement.
// A class that's responsible for the Player Controller component.
//-----------------------------------------------------------------------------

#include <wolf.h>
#include "VelocityComponent.h"

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

    PlayerController() = default;
    PlayerController(wolf::Transform2D* pTransform, VelocityComponent* pVelocity);

    // Update function called every frame
    void Update(float delta);

private:
    // Handle movement input (WASD)
    void HandleMovement(float delta);

    // Handle rolling input (Spacebar)
    void HandleRolling(float delta);

    // Handle jumping input (J key)
    void HandleJumping(float delta);

    // Manage state transitions
    void UpdatePlayerState();

    // Direction for rolling
    glm::vec2 GetRollDirection() const;

    // Member variables, transform and our velocity
    wolf::Transform2D* m_pTransform = nullptr;
    VelocityComponent* m_pVelocity = nullptr;

    // Player action
    PlayerAction m_action = PlayerAction::NONE;

    // Movement variables
    float m_moveSpeed = 200.0f;

    // Rolling variables
    bool m_isRolling = false;
    float m_rollSpeed = 400.0f;
    float m_rollTimer = 0.0f;
    float m_rollDuration = 0.5f; // Duration of the roll

    // Jumping variables
    bool m_isJumping = false;
    float m_jumpHeight = 10.0f;
    float m_jumpSpeed = 300.0f;
    float m_jumpTimer = 0.0f;

    glm::vec2 m_lastDirection = glm::vec2(0.0f);
};