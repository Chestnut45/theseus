#include "PlayerController.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 1.4, updated to encapsulate VelocityComponent update.
// A class that's responsible for the Player Controller component.
//-----------------------------------------------------------------------------

// Constructor
PlayerController::PlayerController(wolf::Transform2D* pTransform, VelocityComponent* pVelocity)
    : m_pTransform(pTransform), m_pVelocity(pVelocity)
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

    // Update the VelocityComponent (encapsulated within PlayerController)
    if (m_pVelocity)
    {
        m_pVelocity->Update(delta);  // Update the velocity and position
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
        m_pVelocity->SetVelocity(direction * m_moveSpeed); // Set velocity

        // Store the last known direction for rolling
        m_lastDirection = direction;
        m_action = PlayerAction::WALKING;
    }
    else if (m_action == PlayerAction::WALKING)
    {
        // Stop movement if no input
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        m_action = PlayerAction::NONE;
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        // Only move in the roll direction, no new input allowed
        glm::vec2 rollDirection = m_lastDirection;
        m_pVelocity->SetVelocity(rollDirection * m_rollSpeed);

        m_rollTimer -= delta;
        if (m_rollTimer <= 0)
        {
            m_isRolling = false;
            m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop rolling
            m_action = PlayerAction::NONE;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && !m_isRolling)
    {
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
        // Simulate jumping by setting upward velocity
        m_pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed));
        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_action = PlayerAction::NONE;  // Reset to idle after jump
            m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop upward velocity
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