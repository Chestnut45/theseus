#include "PlayerController.h"

// Constructor
PlayerController::PlayerController(wolf::Transform2D* pTransform)
    : m_pTransform(pTransform)
{
}

// Update function called every frame
void PlayerController::Update(float delta)
{
    // Handle movement
    HandleMovement(delta);

    // Handle rolling
    HandleRolling(delta);

    // Handle jumping
    HandleJumping(delta);
}

// Handle movement input (WASD)
void PlayerController::HandleMovement(float delta)
{
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) {
        m_pTransform->Translate(glm::vec2(0, m_moveSpeed * delta));
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) {
        m_pTransform->Translate(glm::vec2(0, -m_moveSpeed * delta));
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) {
        m_pTransform->Translate(glm::vec2(-m_moveSpeed * delta, 0));
    }
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) {
        m_pTransform->Translate(glm::vec2(m_moveSpeed * delta, 0));
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling) {
        glm::vec2 rollDirection = GetRollDirection();
        m_pTransform->Translate(rollDirection * m_rollSpeed * delta);

        m_rollTimer -= delta;
        if (m_rollTimer <= 0) {
            m_isRolling = false;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)) {
        // Start rolling
        m_isRolling = true;
        m_rollTimer = m_rollDuration;
    }
}

// Handle jumping input (optional, e.g., J key)
void PlayerController::HandleJumping(float delta)
{
    if (m_isJumping) {
        // Simulate jumping (optional, simple Y-axis translation)
        m_pTransform->Translate(glm::vec2(0, m_jumpSpeed * delta));
        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0) {
            m_isJumping = false;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J)) {
        // Start jump
        m_isJumping = true;
        m_jumpTimer = m_jumpHeight / m_jumpSpeed;
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
    if (glm::length(direction) > 0.0f) {
        return glm::normalize(direction);
    }
    return glm::vec2(0.0f);
}
