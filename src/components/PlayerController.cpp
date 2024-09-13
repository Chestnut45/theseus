#include "PlayerController.h"
//-----------------------------------------------------------------------------
// File:			PlayerController.h
// Original Author:	Youssef Ashraf
// ver 1.2, updated player state conditions and set constraints.
// A class that's responsible for the Player Controller component.
//-----------------------------------------------------------------------------
// Constructor
PlayerController::PlayerController(wolf::Transform2D* pTransform)
    : m_pTransform(pTransform)
{
}

// Update function called every frame
void PlayerController::Update(float delta)
{
    // Handle player states based on current action
    switch (m_action)
    {
        case PlayerAction::ROLLING:
            HandleRolling(delta);
            break;

        case PlayerAction::JUMPING:
            HandleJumping(delta);
            HandleMovement(delta);  // Allow movement while jumping
            break;

        case PlayerAction::WALKING:
        case PlayerAction::NONE:
        default:
            HandleMovement(delta);
            HandleRolling(delta);   // Rolling can interrupt walking
            HandleJumping(delta);   // Allow jumping while walking
            break;
    }
}

// Handle movement input (WASD)
void PlayerController::HandleMovement(float delta)
{
    if (m_action == PlayerAction::ROLLING) return; // Disable movement during roll

    glm::vec2 direction(0.0f);
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) {
        direction.y += 1.0f;
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) {
        direction.y -= 1.0f;
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) {
        direction.x -= 1.0f;
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) {
        direction.x += 1.0f;
    }

    if (glm::length(direction) > 0.0f)
    {
        // Normalize direction for smooth movement
        direction = glm::normalize(direction);
        m_pTransform->Translate(direction * m_moveSpeed * delta);

        // Store the last known direction for rolling
        m_lastDirection = direction;
        m_action = PlayerAction::WALKING;
    }
    else if (m_action == PlayerAction::WALKING)
    {
        m_action = PlayerAction::NONE; // If no input, switch to idle (NONE)
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        // Only move in the roll direction, no new input allowed
        glm::vec2 rollDirection = m_lastDirection;
        m_pTransform->Translate(rollDirection * m_rollSpeed * delta);

        m_rollTimer -= delta;
        if (m_rollTimer <= 0)
        {
            m_isRolling = false;
            m_action = PlayerAction::NONE;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && !m_isRolling)
    {
        // Check if we have a valid direction, otherwise use the last known direction
        glm::vec2 rollDirection = GetRollDirection();
        if (glm::length(rollDirection) > 0.0f)
        {
            m_lastDirection = rollDirection; // Update last known direction
        }

        // Start rolling in the last known direction (can be stationary if no input)
        m_isRolling = true;
        m_rollTimer = m_rollDuration;
        m_action = PlayerAction::ROLLING;
    }
}

// Handle jumping input (J key)
void PlayerController::HandleJumping(float delta)
{
    if (m_action == PlayerAction::ROLLING) return; // Disable jumping during roll

    if (m_isJumping)
    {
        // Simulate jumping (simple Y-axis translation)
        m_pTransform->Translate(glm::vec2(0, m_jumpSpeed * delta));
        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_action = PlayerAction::NONE;  // Reset to idle after jump
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J) && !m_isJumping)
    {
        // Start jump
        m_isJumping = true;
        m_jumpTimer = m_jumpHeight / m_jumpSpeed;
        m_action = PlayerAction::JUMPING;
    }
}

// Get current roll direction (based on movement input)
glm::vec2 PlayerController::GetRollDirection() const
{
    glm::vec2 direction(0.0f, 0.0f);
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) direction.y += 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) direction.y -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) direction.x -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) direction.x += 1.0f;
    
    // Return normalized direction or zero if no input
    return glm::length(direction) > 0.0f ? glm::normalize(direction) : glm::vec2(0.0f);
}