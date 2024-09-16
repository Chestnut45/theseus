#include "PlayerController.h"
#include "VelocityComponent.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 1.7, updated to use member variables for components,
// Player State Management, Velocity-Based Movement, Decoupled,
//-----------------------------------------------------------------------------

// Constructor
PlayerController::PlayerController()
{
}

// Update function called every frame
void PlayerController::Update(float delta)
{
    // Grab current transform and velocity components from the game object
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = pGameObject->GetComponent<VelocityComponent>();
    }

    // Only update if both components exist
    if (m_pTransform && m_pVelocity)
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
}

// Handle movement input (WASD)
void PlayerController::HandleMovement(float delta)
{
    if (m_action == PlayerAction::ROLLING) return; // Disable movement during roll

    glm::vec2 direction(0.0f);
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) direction.y += 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) direction.y -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) direction.x -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) direction.x += 1.0f;

    if (glm::length(direction) > 0.0f)
    {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_moveSpeed); // Set velocity 
        m_lastDirection = direction;
        m_action = PlayerAction::WALKING;
    }
    else if (m_action == PlayerAction::WALKING)
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop movement
        m_action = PlayerAction::NONE;
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        glm::vec2 rollDirection = m_lastDirection;
        m_pVelocity->SetVelocity(rollDirection * m_rollSpeed); // Continue rolling in last known direction

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
        m_pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed)); // Set upward velocity for jump

        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_action = PlayerAction::NONE;
            m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop upward velocity
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J) && !m_isJumping)
    {
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