#pragma once

#include <wolf.h>

class PlayerController : public wolf::BaseComponent
{
public:
    enum class PlayerState
    {
        IDLE,
        WALKING,
        JUMPING,
        ROLLING
    };

    PlayerController() = default;
    PlayerController(wolf::Transform2D* pTransform);

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

    // Member variables
    wolf::Transform2D* m_pTransform = nullptr;

    // Player state
    PlayerState m_state = PlayerState::IDLE;

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
};
