#include "PlayerController.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "HurtboxComponent.h"
#include "HitboxComponent.h"
#include <iostream>
#include <W_Input.h>

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// Modifications: Aurora Ryder, etc.
// ver 1.9. 
//-----------------------------------------------------------------------------

// Constructor
PlayerController::PlayerController()
{
    // Initialization of default variables can go here if needed
}

void PlayerController::SetAnimationComponent(AnimatedSprite2D* animComponent)
{
    m_pAnimComponent = animComponent;
}

void PlayerController::LateInitialize()
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject) return;

    if (!m_pAnimComponent)
    {
        m_pAnimComponent = &pGameObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
    }

    InitializeAnimations();
}


void PlayerController::InitializeAnimations()
{
    if (!m_pAnimComponent) return;

    // Add all animations for the player character
    m_pAnimComponent->AddAnimation("WalkSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, true);
    m_pAnimComponent->AddAnimation("WalkEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, true);
    m_pAnimComponent->AddAnimation("WalkNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, true);
    m_pAnimComponent->AddAnimation("WalkWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, true);

    m_pAnimComponent->AddAnimation("StandSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    m_pAnimComponent->AddAnimation("StandEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 9, false);
    m_pAnimComponent->AddAnimation("StandNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 17, false);
    m_pAnimComponent->AddAnimation("StandWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 25, false);

    m_pAnimComponent->AddAnimation("AttackSouth", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, false);
    m_pAnimComponent->AddAnimation("AttackEast", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, false);
    m_pAnimComponent->AddAnimation("AttackNorth", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, false);
    m_pAnimComponent->AddAnimation("AttackWest", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, false);

    // Set the origin to the center of the frame for proper rendering alignment
    m_pAnimComponent->SetOriginToCenterOfFrame();

    // Set the default animation to idle south
    m_pAnimComponent->SetAnimation("StandSouth");
}

void PlayerController::Update(float delta)
{
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        // Lazy initialization of components if not set yet
        m_pTransform = m_pTransform ? m_pTransform : pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = m_pVelocity ? m_pVelocity : pGameObject->GetComponent<VelocityComponent>();

        if (!m_pAnimComponent)
        {
            LateInitialize();
        }
    }

    // Ensure essential components are initialized before updating
    if (!m_pTransform || !m_pVelocity || !m_pAnimComponent)
    {
        std::cerr << "Error: Missing essential components in PlayerController!" << std::endl;
        return;
    }

    // Update the player state and animations based on input and conditions
    HandleMovement(delta);
    HandleRolling(delta);
    HandleJumping(delta);
    HandleAttacking(delta);

    // Regenerate stamina if not performing specific actions
    if (!m_isRolling && !m_isAttacking)
    {
        RegenerateStamina(delta);
    }

    // Set the correct animation based on player state
    SetAnimationBasedOnState();
}
void PlayerController::AddHeldKey(int key)
{
    // If the key is not already in the list, add it
    if (std::find(m_heldKeys.begin(), m_heldKeys.end(), key) == m_heldKeys.end())
    {
        m_heldKeys.push_back(key);
    }
}
void PlayerController::RemoveHeldKey(int key)
{
    m_heldKeys.erase(std::remove(m_heldKeys.begin(), m_heldKeys.end(), key), m_heldKeys.end());
}
PlayerController::PlayerDirection PlayerController::GetDirectionFromHeldKeys() const
{
    // Track the held keys and determine the direction
    bool wHeld = wolf::Input::IsKeyHeld(GLFW_KEY_W);
    bool sHeld = wolf::Input::IsKeyHeld(GLFW_KEY_S);
    bool aHeld = wolf::Input::IsKeyHeld(GLFW_KEY_A);
    bool dHeld = wolf::Input::IsKeyHeld(GLFW_KEY_D);

    if (wHeld && aHeld) return PlayerDirection::NORTH_WEST;
    if (wHeld && dHeld) return PlayerDirection::NORTH_EAST;
    if (sHeld && aHeld) return PlayerDirection::SOUTH_WEST;
    if (sHeld && dHeld) return PlayerDirection::SOUTH_EAST;
    if (wHeld) return PlayerDirection::NORTH;
    if (sHeld) return PlayerDirection::SOUTH;
    if (aHeld) return PlayerDirection::WEST;
    if (dHeld) return PlayerDirection::EAST;

    return PlayerDirection::NONE;  // Default case
}
void PlayerController::HandleMovement(float delta)
{
    // Vector to store movement direction
    glm::vec2 direction = glm::vec2(0.0f);

    // Track which keys are being pressed and released
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_W)) AddHeldKey(GLFW_KEY_W);
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_S)) AddHeldKey(GLFW_KEY_S);
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_A)) AddHeldKey(GLFW_KEY_A);
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_D)) AddHeldKey(GLFW_KEY_D);

    if (wolf::Input::IsKeyJustUp(GLFW_KEY_W)) RemoveHeldKey(GLFW_KEY_W);
    if (wolf::Input::IsKeyJustUp(GLFW_KEY_S)) RemoveHeldKey(GLFW_KEY_S);
    if (wolf::Input::IsKeyJustUp(GLFW_KEY_A)) RemoveHeldKey(GLFW_KEY_A);
    if (wolf::Input::IsKeyJustUp(GLFW_KEY_D)) RemoveHeldKey(GLFW_KEY_D);

    // Determine movement direction based on currently held keys
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) direction.y += 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) direction.y -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) direction.x -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) direction.x += 1.0f;

    // If no movement input is detected, reset velocity and set to idle state
    if (direction == glm::vec2(0.0f))
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        if (!m_isAttacking && !m_isRolling)
        {
            m_action = PlayerAction::NONE;
            SetAnimationBasedOnState();  // Set idle animation
        }
        return;  // Skip further processing if no movement input
    }

    // Normalize and set velocity based on direction and speed
    direction = glm::normalize(direction);

    // Determine the facing direction based on the last pressed keys
    m_lastDirectionEnum = GetDirectionFromHeldKeys();

    // Set the player's velocity based on the direction and movement speed
    m_pVelocity->SetVelocity(direction * m_moveSpeed);

    // Update state to walking if not rolling or jumping
    if (!m_isRolling && !m_isJumping)
    {
        m_action = PlayerAction::WALKING;
    }

    // Set animation based on state and last known direction
    SetAnimationBasedOnState();
}


void PlayerController::SetAnimationBasedOnState()
{
    if (m_isAttacking || m_isRolling || m_isJumping) return;  // Skip if performing an action like attacking, rolling, or jumping

    // Determine the animation based on the current state and last direction
    std::string animationName;

    switch (m_action)
    {
        case PlayerAction::WALKING:
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH: animationName = "WalkSouth"; break;
                case PlayerDirection::EAST: animationName = "WalkEast"; break;
                case PlayerDirection::NORTH: animationName = "WalkNorth"; break;
                case PlayerDirection::WEST: animationName = "WalkWest"; break;
                case PlayerDirection::NORTH_EAST: animationName = "WalkNorth"; break;
                case PlayerDirection::NORTH_WEST: animationName = "WalkNorth"; break;
                case PlayerDirection::SOUTH_EAST: animationName = "WalkSouth"; break;
                case PlayerDirection::SOUTH_WEST: animationName = "WalkSouth"; break;
                default: animationName = "WalkSouth"; break;
            }
            break;

        case PlayerAction::NONE:  // Idle state
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH: animationName = "StandSouth"; break;
                case PlayerDirection::EAST: animationName = "StandEast"; break;
                case PlayerDirection::NORTH: animationName = "StandNorth"; break;
                case PlayerDirection::WEST: animationName = "StandWest"; break;
                case PlayerDirection::NORTH_EAST: animationName = "StandEast"; break;
                case PlayerDirection::NORTH_WEST: animationName = "StandWest"; break;
                case PlayerDirection::SOUTH_EAST: animationName = "StandEast"; break;
                case PlayerDirection::SOUTH_WEST: animationName = "StandWest"; break;
                default: animationName = "StandSouth"; break;
            }
            break;

        default:
            return;  // No need to change animation for other states
    }

    // Set the animation and log the change
    // std::cout << "Set animation to: " << animationName << " based on state: " << static_cast<int>(m_action) 
    //           << " and direction: " << m_lastDirectionEnum << std::endl;
    m_pAnimComponent->SetAnimation(animationName);
    // std::cout << "Successfully switched to animation: " << animationName << std::endl;

    // Center the origin for consistent rendering
    m_pAnimComponent->SetOriginToCenterOfFrame();
}

std::string PlayerController::GetAttackAnimationForDirection(PlayerDirection direction) const
{
    switch (direction)
    {
    case PlayerDirection::SOUTH:        return "AttackSouth";
    case PlayerDirection::EAST:         return "AttackEast";
    case PlayerDirection::NORTH:        return "AttackNorth";
    case PlayerDirection::WEST:         return "AttackWest";
    case PlayerDirection::NORTH_EAST:   return "AttackEast";
    case PlayerDirection::NORTH_WEST:   return "AttackWest";
    case PlayerDirection::SOUTH_EAST:   return "AttackEast";
    case PlayerDirection::SOUTH_WEST:   return "AttackWest";
    default:                            return "AttackSouth"; // Default to South attack if no valid direction
    }
}
PlayerController::PlayerDirection PlayerController::GetDirectionFromVector(const glm::vec2& direction) const
{
    if (direction == glm::vec2(0.0f, 0.0f))
        return PlayerDirection::NONE;

    // Normalize the direction for consistent comparison
    glm::vec2 normalizedDir = glm::normalize(direction);

    // Determine and return the correct enum based on normalized direction vector
    if (normalizedDir.x > 0.0f && normalizedDir.y > 0.0f) return PlayerDirection::NORTH_EAST;
    if (normalizedDir.x > 0.0f && normalizedDir.y < 0.0f) return PlayerDirection::SOUTH_EAST;
    if (normalizedDir.x < 0.0f && normalizedDir.y > 0.0f) return PlayerDirection::NORTH_WEST;
    if (normalizedDir.x < 0.0f && normalizedDir.y < 0.0f) return PlayerDirection::SOUTH_WEST;
    if (normalizedDir.x > 0.0f) return PlayerDirection::EAST;
    if (normalizedDir.x < 0.0f) return PlayerDirection::WEST;
    if (normalizedDir.y > 0.0f) return PlayerDirection::NORTH;
    if (normalizedDir.y < 0.0f) return PlayerDirection::SOUTH;

    return PlayerDirection::NONE;
}


void PlayerController::HandleAttacking(float delta)
{
    if (wolf::Input::IsLMBJustDown() && !m_isAttacking)
    {
        StartAttack();  // Start the attack
        return;
    }

    // Check if the attack animation is complete and transition to another state if it is
    if (m_isAttacking)
    {
        UpdateAttackState(delta);
    }
}
void PlayerController::ApplyDamageToEnemy()
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject || !m_pTransform || !m_pHitboxManager || !m_pHurtboxManager) return;

    auto* pPlayerHitbox = pGameObject->GetComponent<HitboxComponent>();
    if (!pPlayerHitbox) return;

    for (auto&& [entity, enemyController] : GetGameObject()->GetScene().Each<EnemyController>())
    {
        auto* enemyHurtbox = enemyController.GetGameObject()->GetComponent<HurtboxComponent>();
        if (!enemyHurtbox) continue;

        if (m_pHitboxManager->IsColliding(pPlayerHitbox, enemyHurtbox))
        {
            auto* enemyHealth = enemyController.GetGameObject()->GetComponent<HealthComponent>();
            if (enemyHealth)
            {
                enemyHealth->Damage(m_attackDamage);
                std::cout << "Player attacked enemy for " << m_attackDamage << " damage!" << std::endl;
            }
        }
    }
}
void PlayerController::SetManagers(HitboxManager* pHitboxManager, HurtboxManager* pHurtboxManager)
{
    m_pHitboxManager = pHitboxManager;
    m_pHurtboxManager = pHurtboxManager;
}
void PlayerController::StartAttack()
{
    m_isAttacking = true;
    m_action = PlayerAction::ATTACKING;
    m_attackTimer.Restart();  // Restart the attack timer
    m_animationFinished = false;

    // Set animation based on current direction
    std::string attackAnimation = GetAttackAnimationForDirection(m_lastDirectionEnum);
    m_pAnimComponent->SetTexture("data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f));
    m_pAnimComponent->SetAnimation(attackAnimation, 0);  // Start from frame 0

    std::cout << "Attack initiated in direction: " << m_lastDirectionEnum << " with animation: " << attackAnimation << std::endl;

    m_pAnimComponent->SetOriginToCenterOfFrame();  // Center the origin
}
void PlayerController::UpdateAttackState(float delta)
{
    // Check if the animation is near completion or fully complete
    if (m_pAnimComponent->IsAnimationComplete())
    {
        std::cout << "Attack animation completed in direction: " << m_lastDirectionEnum << std::endl;
        m_animationFinished = true;
    }

    if (m_animationFinished)
    {
        // Reset attacking state and switch to appropriate idle/walk state
        m_isAttacking = false;
        m_action = glm::length(m_pVelocity->GetVelocity()) < 0.01f ? PlayerAction::NONE : PlayerAction::WALKING;

        // Restore the texture and set the appropriate animation based on the state
        m_pAnimComponent->SetTexture("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f));
        SetAnimationBasedOnState();  // Set animation based on state and direction
    }
}
// Handle movement input (WASD)


void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        glm::vec2 rollDirection = GetDirectionVector(m_lastDirectionEnum);
        m_pVelocity->SetVelocity(rollDirection * m_rollSpeed);
        m_rollTimer -= delta;

        if (m_rollTimer <= 0)
        {
            m_isRolling = false;
            m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Stop rolling
            m_action = PlayerAction::NONE;
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && !m_isRolling && m_stamina >= 15.0f)
    {
        m_isRolling = true;
        m_rollTimer = m_rollDuration;
        m_action = PlayerAction::ROLLING;
        m_stamina -= 25.0f;
        m_staminaRegenTimer.Restart();
    }
}

void PlayerController::HandleJumping(float delta)
{
    if (m_action == PlayerAction::ROLLING || m_isAttacking) return; // Skip jumping during roll or attack

    if (m_isJumping)
    {
        m_pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed)); 
        m_jumpTimer -= delta;

        if (m_jumpTimer <= 0)
        {
            m_isJumping = false;
            m_action = PlayerAction::NONE;
            m_pVelocity->SetVelocity(glm::vec2(0.0f));
        }
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J) && !m_isJumping)
    {
        m_isJumping = true;
        m_jumpTimer = m_jumpHeight / m_jumpSpeed;
        m_action = PlayerAction::JUMPING;
    }
}
void PlayerController::RegenerateStamina(float delta)
{
    if (m_stamina < m_maxStamina && m_staminaRegenTimer.Elapsed() >= m_staminaRegenDelay)
    {
        m_stamina += m_staminaRegenRate * delta;
        m_stamina = std::min(m_stamina, m_maxStamina);
    }
}

// Get the direction enum based on movement input

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

    // Draw the health bar below the stamina bar
    auto* healthComponent = GetGameObject()->GetComponent<HealthComponent>();
    if (healthComponent)
    {
        // Adjust position to be below the stamina bar
        screenPos.y += 15.0f;

        ImGui::SetNextWindowPos(ImVec2(screenPos.x - 25.0f, screenPos.y)); // Center the bar below the stamina
        ImGui::SetNextWindowSize(ImVec2(50.0f, 10.0f)); // Set window size

        ImGui::Begin("##HealthBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

        // Calculate health percentage
        float healthPercent = healthComponent->GetHealth() / healthComponent->GetMaxHealth();

        // Set the color to red for the health bar
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
        ImGui::ProgressBar(healthPercent, ImVec2(-1, 0)); // Full width, default height
        ImGui::PopStyleColor();

        ImGui::End();
    }
}

std::ostream& operator<<(std::ostream& os, const PlayerController::PlayerDirection& direction) {
    switch (direction) {
        case PlayerController::PlayerDirection::NONE:        os << "NONE"; break;
        case PlayerController::PlayerDirection::NORTH:       os << "NORTH"; break;
        case PlayerController::PlayerDirection::NORTH_EAST:  os << "NORTH_EAST"; break;
        case PlayerController::PlayerDirection::EAST:        os << "EAST"; break;
        case PlayerController::PlayerDirection::SOUTH_EAST:  os << "SOUTH_EAST"; break;
        case PlayerController::PlayerDirection::SOUTH:       os << "SOUTH"; break;
        case PlayerController::PlayerDirection::SOUTH_WEST:  os << "SOUTH_WEST"; break;
        case PlayerController::PlayerDirection::WEST:        os << "WEST"; break;
        case PlayerController::PlayerDirection::NORTH_WEST:  os << "NORTH_WEST"; break;
        default:                                             os << "UNKNOWN"; break;
    }
    return os;
}