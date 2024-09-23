#include "PlayerController.h"
#include "VelocityComponent.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 1.7, updated to use member variables for components,
// Player State Management, Velocity-Based Movement, Decoupled, test.
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
    if (m_stamina < m_maxStamina)
    {
        // Check if 2 seconds have passed since the last stamina use
        if (m_staminaRegenTimer.Elapsed() >= 2.0)
        {
            // Regenerate stamina
            m_stamina += m_staminaRegenRate * delta;
            if (m_stamina > m_maxStamina)
                m_stamina = m_maxStamina;
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
        if (m_stamina >= 15.0f)
        {
            m_isRolling = true;
            m_rollTimer = m_rollDuration;
            m_action = PlayerAction::ROLLING;

            // Consume stamina
            m_stamina -= 25.0f;
            if (m_stamina < 0.0f)
                m_stamina = 0.0f;

            // Start/reset the stamina regeneration timer
            m_staminaRegenTimer.Restart();
        }
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
void PlayerController::Render()
{
    if (!m_pTransform) return;

    // Get the player's position
    glm::vec2 worldPos = m_pTransform->GetGlobalPosition();

    // Get the active camera
    wolf::Camera2D* camera = GetGameObject()->GetScene().GetActiveCamera();
    if (!camera) return;

    // Get the combined view-projection matrix
    glm::mat4 viewProj = camera->GetMatrix();

    // Convert world position to clip space
    glm::vec4 worldPos4(worldPos.x, worldPos.y, 0.0f, 1.0f);
    glm::vec4 clipSpacePos = viewProj * worldPos4;

    // Normalize device coordinates
    glm::vec3 ndcSpacePos = clipSpacePos / clipSpacePos.w;

    // Convert to screen space
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    glm::vec2 screenPos = glm::vec2(
        (ndcSpacePos.x * 0.5f + 0.5f) * displaySize.x,
        (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * displaySize.y
    );

    // Adjust position to be above the player
    screenPos.y -= 60.0f;

    // Draw the stamina bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x - 25.0f, screenPos.y)); // Center the bar above the player
    ImGui::SetNextWindowSize(ImVec2(50.0f, 10.0f)); // Set window size

    ImGui::Begin("##StaminaBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    // Calculate stamina percentage
    float staminaPercent = m_stamina / m_maxStamina;

    // Draw the stamina bar
    ImGui::ProgressBar(staminaPercent, ImVec2(-1, 0)); // Full width, default height

    ImGui::End();
}