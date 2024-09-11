#include "PlayerController.h"

// Constructor
PlayerController::PlayerController(wolf::Transform2D* pTransform)
    : m_pTransform(pTransform)
{
}

// Update function called every frame
void PlayerController::Update(float delta)
{
    // Update the player state
    UpdatePlayerState();

    // Handle player states
    switch (m_state)
    {
        case PlayerState::WALKING:
            HandleMovement(delta);
            break;
        case PlayerState::ROLLING:
            HandleRolling(delta);
            break;
        case PlayerState::JUMPING:
            HandleJumping(delta);
            break;
        case PlayerState::IDLE:
        default:
            // No movement in idle state, but handle input to transition to other states
            HandleMovement(delta);
            HandleRolling(delta);
            HandleJumping(delta);
            break;
    }
}

// Handle movement input (WASD)
void PlayerController::HandleMovement(float delta)
{
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
        direction = glm::normalize(direction);
        m_pTransform->Translate(direction * m_moveSpeed * delta);
        m_state = PlayerState::WALKING;
    }
    else if (m_state == PlayerState::WALKING)
    {
        m_state = PlayerState::IDLE; // If no input, switch to idle
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        glm::vec2 rollDirection = GetRollDirection();
        m_pTransform->Translate(rollDirection * m_rollSpeed * delta);

        m_rollTimer -= delta;
        if (m_rollTimer <= 0)
        {
            m_isRolling = false;
            m_state = PlayerState::IDLE;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && !m_isRolling)
    {
        // Start rolling
        m_isRolling = true;
        m_rollTimer = m_rollDuration;
        m_state = PlayerState::ROLLING;
    }
}

// Handle jumping input (J key)
void PlayerController::HandleJumping(float delta)
{
    if (m_isJumping)
    {
        // Simulate jumping (simple Y-axis translation)
        m_pTransform->Translate(glm::vec2(0, m_jumpSpeed * delta));
        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_state = PlayerState::IDLE;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J) && !m_isJumping)
    {
        // Start jump
        m_isJumping = true;
        m_jumpTimer = m_jumpHeight / m_jumpSpeed;
        m_state = PlayerState::JUMPING;
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

// Update player state logic (for better state transitions)
void PlayerController::UpdatePlayerState()
{
    if (m_isRolling)
    {
        m_state = PlayerState::ROLLING;
    }
    else if (m_isJumping)
    {
        m_state = PlayerState::JUMPING;
    }
    else if (glm::length(GetRollDirection()) > 0.0f)
    {
        m_state = PlayerState::WALKING;
    }
    else
    {
        m_state = PlayerState::IDLE;
    }
}
