#include "PlayerController.h"
#include "VelocityComponent.h"

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// Modifications: D'Anyil Landry, Aurora, Nguyễn Minh Nhật, Aurora Ryder
// ver 1.8, updated to use member variables for components,
// Player State Management, Velocity-Based Movement, Decoupled, Enum Direction, Direction Vector, Stamina
//-----------------------------------------------------------------------------

// Constructor
PlayerController::PlayerController()
{
}

void PlayerController::Update(float delta)
{
    // Grab current transform and velocity components from the game object
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        m_pTransform = pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = pGameObject->GetComponent<VelocityComponent>();
        m_pAnim = pGameObject->GetComponent<AnimatedSprite2D>();
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
                HandleMovement(delta);
                HandleRolling(delta);   // Rolling can interrupt walking
                HandleJumping(delta);   // Allow jumping while walking
                break;
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

    PlayerDirection newDirection = GetRollDirection();
    glm::vec2 direction = GetDirectionVector(newDirection);

    if (direction != glm::vec2(0.0f))
    {
        m_pVelocity->SetVelocity(direction * m_moveSpeed); // Set velocity 
        m_lastDirectionEnum = newDirection;
        m_action = PlayerAction::WALKING;
        switch (newDirection) {
            case PlayerDirection::SOUTH:
                m_pAnim->SetAnimation("WalkSouth");
                break;
            case PlayerDirection::SOUTH_EAST:
                m_pAnim->SetAnimation("WalkEast");
                break;
            case PlayerDirection::EAST:
                m_pAnim->SetAnimation("WalkEast");
                break;
            case PlayerDirection::NORTH_EAST:
                m_pAnim->SetAnimation("WalkEast");
                break;
            case PlayerDirection::NORTH:
                m_pAnim->SetAnimation("WalkNorth");
            break;
            case PlayerDirection::NORTH_WEST:
                m_pAnim->SetAnimation("WalkWest");
                break;
            case PlayerDirection::WEST:
                m_pAnim->SetAnimation("WalkWest");
                break;
            case PlayerDirection::SOUTH_WEST:
                m_pAnim->SetAnimation("WalkWest");
                break;
            default:
                m_pAnim->SetAnimation("WalkSouth");
                break;
        }
    }
    else if (m_action == PlayerAction::WALKING || m_action == PlayerAction::NONE)
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop movement
        m_action = PlayerAction::NONE;
        switch (m_lastDirectionEnum) {
            case PlayerDirection::SOUTH:
                m_pAnim->SetAnimation("StandSouth");
                break;
            case PlayerDirection::SOUTH_EAST:
                m_pAnim->SetAnimation("StandEast");
                break;
            case PlayerDirection::EAST:
                m_pAnim->SetAnimation("StandEast");
                break;
            case PlayerDirection::NORTH_EAST:
                m_pAnim->SetAnimation("StandEast");
                break;
            case PlayerDirection::NORTH:
                m_pAnim->SetAnimation("StandNorth");
            break;
            case PlayerDirection::NORTH_WEST:
                m_pAnim->SetAnimation("StandWest");
                break;
            case PlayerDirection::WEST:
                m_pAnim->SetAnimation("StandWest");
                break;
            case PlayerDirection::SOUTH_WEST:
                m_pAnim->SetAnimation("StandWest");
                break;
            default:
                m_pAnim->SetAnimation("StandSouth");
                break;
        }
    }
}

// Handle rolling input (Spacebar)
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        glm::vec2 rollDirection = GetDirectionVector(m_lastDirectionEnum);
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

            // Use the last direction for the roll
            glm::vec2 rollDirection = GetDirectionVector(m_lastDirectionEnum);

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

// Get the direction enum based on movement input
PlayerController::PlayerDirection PlayerController::GetRollDirection() const
{
    glm::vec2 direction(0.0f, 0.0f);
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) direction.y += 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) direction.y -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) direction.x -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) direction.x += 1.0f;

    // Normalize the direction if there's movement
    if (direction != glm::vec2(0.0f, 0.0f))
        direction = glm::normalize(direction);

    // Determine and return the correct enum based on direction
    if (direction.x == 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH;
    if (direction.x > 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH_EAST;
    if (direction.x > 0.0f && direction.y == 0.0f) return PlayerDirection::EAST;
    if (direction.x > 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH_EAST;
    if (direction.x == 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH;
    if (direction.x < 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH_WEST;
    if (direction.x < 0.0f && direction.y == 0.0f) return PlayerDirection::WEST;
    if (direction.x < 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH_WEST;
    
    return PlayerDirection::NONE; // Default case
}


// Convert direction enum to glm::vec2 for movement
glm::vec2 PlayerController::GetDirectionVector(PlayerDirection direction) const
{
    switch (direction)
    {
        case PlayerDirection::NORTH:       return glm::vec2(0.0f, 1.0f);
        case PlayerDirection::NORTH_EAST:  return glm::vec2(1.0f, 1.0f);
        case PlayerDirection::EAST:        return glm::vec2(1.0f, 0.0f);
        case PlayerDirection::SOUTH_EAST:  return glm::vec2(1.0f, -1.0f);
        case PlayerDirection::SOUTH:       return glm::vec2(0.0f, -1.0f);
        case PlayerDirection::SOUTH_WEST:  return glm::vec2(-1.0f, -1.0f);
        case PlayerDirection::WEST:        return glm::vec2(-1.0f, 0.0f);
        case PlayerDirection::NORTH_WEST:  return glm::vec2(-1.0f, 1.0f);
        default:                           return glm::vec2(0.0f, 0.0f); 
    }
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