#include "PlayerController.h"
#include "VelocityComponent.h"
#include "W_Transform2D.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 1.7, updated to use GetGameObject() for dynamic component retrieval.
//-----------------------------------------------------------------------------

void PlayerController::Update(float delta)
{
    // Dynamically get components on the same GameObject
    auto* pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();
    auto* pVelocity = GetGameObject()->GetComponent<VelocityComponent>();

    if (pTransform && pVelocity)
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
        auto* pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
        if (pVelocity)
        {
            pVelocity->SetVelocity(direction * m_moveSpeed); // Set velocity
        }

        m_lastDirection = direction;
        m_action = PlayerAction::WALKING;
    }
    else if (m_action == PlayerAction::WALKING)
    {
        auto* pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
        if (pVelocity)
        {
            pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop movement
        }
        m_action = PlayerAction::NONE;
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        glm::vec2 rollDirection = m_lastDirection;
        auto* pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
        if (pVelocity)
        {
            pVelocity->SetVelocity(rollDirection * m_rollSpeed); // Continue rolling in last known direction
        }

        m_rollTimer -= delta;
        if (m_rollTimer <= 0)
        {
            m_isRolling = false;
            if (pVelocity)
            {
                pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop rolling
            }
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
        auto* pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
        if (pVelocity)
        {
            pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed)); // Set upward velocity for jump
        }

        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_action = PlayerAction::NONE;
            if (pVelocity)
            {
                pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop upward velocity
            }
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