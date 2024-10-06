#include "PlayerController.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "HurtboxComponent.h"
#include "HitboxComponent.h"
#include <W_Input.h>
#include <W_Logging.h>

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 2.0: Optimized and restructured for readability and performance.
//-----------------------------------------------------------------------------

PlayerController::PlayerController() = default;

void PlayerController::SetAnimationComponent(AnimatedSprite2D* animComponent)
{
    m_pAnimComponent = animComponent;
}

void PlayerController::SetManagers(HitboxManager* pHitboxManager, HurtboxManager* pHurtboxManager)
{
    m_pHitboxManager = pHitboxManager;
    m_pHurtboxManager = pHurtboxManager;
}

// Initialize components related to the player
void PlayerController::LateInitialize()
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: PlayerController not attached to GameObject!");
        return;
    }

    if (!m_pAnimComponent)
    {
        m_pAnimComponent = &pGameObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
        if (m_pAnimComponent) InitializeAnimations();

        // Set the default animation to an idle animation
        m_pAnimComponent->SetAnimation("StandSouth");
    }
}

// Add and initialize animations for the player character
void PlayerController::InitializeAnimations()
{
    if (!m_pAnimComponent) return;

    // Define all player animations with their corresponding texture paths and frame indices.
    std::vector<std::tuple<std::string, std::string, int, int>> animations = {
        {"WalkSouth", "data/textures/TheseusWalk-Sheet.png", 1, 8},
        {"WalkEast", "data/textures/TheseusWalk-Sheet.png", 9, 16},
        {"WalkNorth", "data/textures/TheseusWalk-Sheet.png", 17, 24},
        {"WalkWest", "data/textures/TheseusWalk-Sheet.png", 25, 32},
        {"StandSouth", "data/textures/TheseusStand-Sheet.png", 1, 1},
        {"StandEast", "data/textures/TheseusStand-Sheet.png", 2, 2},
        {"StandNorth", "data/textures/TheseusStand-Sheet.png", 3, 3},
        {"StandWest", "data/textures/TheseusStand-Sheet.png", 4, 4},
        {"AttackSouth", "data/textures/TheseusSword-Sheet.png", 1, 8},
        {"AttackEast", "data/textures/TheseusSword-Sheet.png", 9, 16},
        {"AttackNorth", "data/textures/TheseusSword-Sheet.png", 17, 24},
        {"AttackWest", "data/textures/TheseusSword-Sheet.png", 25, 32}
    };

    // Add animations to the component with correct frame ranges
    for (const auto& [name, path, startFrame, endFrame] : animations)
    {
        m_pAnimComponent->AddAnimation(name, path, glm::vec2(32.0f, 32.0f), startFrame, endFrame, true);
    }

    m_pAnimComponent->SetAnimation("StandSouth"); // Default animation set to "StandSouth"
    m_pAnimComponent->SetOriginToCenterOfFrame();
}

// Main update loop for the player controller
void PlayerController::Update(float delta)
{
    if (!m_pTransform) m_pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!m_pVelocity) m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pAnimComponent) LateInitialize();

    if (!m_pTransform || !m_pVelocity || !m_pAnimComponent)
    {
        wolf::Error("PlayerController missing essential components!");
        return;
    }

    HandlePlayerInput(delta);
    RegenerateStamina(delta);
    SetAnimationBasedOnState();
}

// Handle all player inputs and manage states accordingly
void PlayerController::HandlePlayerInput(float delta)
{
    HandleMovement(delta);
    HandleRolling(delta);
    HandleJumping(delta);
    HandleAttacking(delta);
}

// Handle player movement based on input
void PlayerController::HandleMovement(float delta)
{
    if (m_action == PlayerAction::ROLLING) return;

    glm::vec2 direction(0.0f);

    // Track and update currently held keys for smooth directional input
    direction.y += wolf::Input::IsKeyDown(GLFW_KEY_W) ? 1.0f : 0.0f;
    direction.y -= wolf::Input::IsKeyDown(GLFW_KEY_S) ? 1.0f : 0.0f;
    direction.x -= wolf::Input::IsKeyDown(GLFW_KEY_A) ? 1.0f : 0.0f;
    direction.x += wolf::Input::IsKeyDown(GLFW_KEY_D) ? 1.0f : 0.0f;

    if (glm::length(direction) == 0.0f)
    {
        if (!m_isAttacking && !m_isRolling) m_action = PlayerAction::NONE;
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        return;
    }

    direction = glm::normalize(direction);
    m_lastDirectionEnum = GetDirectionFromVector(direction);
    m_pVelocity->SetVelocity(direction * m_moveSpeed);

    if (!m_isRolling && !m_isJumping) m_action = PlayerAction::WALKING;
}

// Manage attack state and animation transitions
void PlayerController::HandleAttacking(float delta)
{
    if (wolf::Input::IsLMBJustDown() && !m_isAttacking) StartAttack();
    if (m_isAttacking) UpdateAttackState(delta);
}

// Handle rolling logic based on player input and stamina
void PlayerController::HandleRolling(float delta)
{
    if (m_isRolling)
    {
        m_rollTimer -= delta;
        if (m_rollTimer <= 0.0f) EndRoll();
        return;
    }

    if (wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && m_stamina >= 15.0f)
    {
        StartRoll();
    }
}

// Manage jumping state transitions
void PlayerController::HandleJumping(float delta)
{
    if (m_isJumping)
    {
        m_jumpTimer -= delta;
        if (m_jumpTimer <= 0.0f) EndJump();
    }
    else if (wolf::Input::IsKeyJustDown(GLFW_KEY_J))
    {
        StartJump();
    }
}

// Set appropriate animation based on player state and direction
void PlayerController::SetAnimationBasedOnState()
{
    if (m_isAttacking || m_isRolling || m_isJumping) return;  // Skip if performing an action like attacking, rolling, or jumping

    std::string animationName;
    std::string texturePath = "data/textures/TheseusWalk-Sheet.png"; // Default to walking texture

    // Determine the correct animation and texture based on state and direction
    switch (m_action)
    {
        case PlayerAction::WALKING:
            animationName = GetWalkAnimationForDirection(m_lastDirectionEnum);
            break;

        case PlayerAction::NONE:  // Idle state
            animationName = GetIdleAnimationForDirection(m_lastDirectionEnum);
            texturePath = "data/textures/TheseusStand-Sheet.png"; // Use the standing texture for idle state
            break;

        default:
            return;  // No need to change animation for other states
    }

    // Check if the desired animation is different from the currently playing one
    if (!animationName.empty() && animationName != m_currentAnimation)
    {
        // Set the correct texture for the animation
        m_pAnimComponent->SetTexture(texturePath, glm::vec2(32.0f, 32.0f));
        
        // Set the animation after switching the texture
        m_pAnimComponent->SetAnimation(animationName);
        
        // Update the current animation name
        m_currentAnimation = animationName;
    }
}

// Utility functions for getting animations based on direction
std::string PlayerController::GetWalkAnimationForDirection(PlayerDirection direction) const
{
    switch (direction)
    {
        case PlayerDirection::SOUTH:       return "WalkSouth";
        case PlayerDirection::EAST:        return "WalkEast";
        case PlayerDirection::NORTH:       return "WalkNorth";
        case PlayerDirection::WEST:        return "WalkWest";
        case PlayerDirection::NORTH_EAST:  return "WalkEast";
        case PlayerDirection::NORTH_WEST:  return "WalkWest";
        case PlayerDirection::SOUTH_EAST:  return "WalkEast";
        case PlayerDirection::SOUTH_WEST:  return "WalkWest";
        default:                           return "WalkSouth";
    }
}

std::string PlayerController::GetIdleAnimationForDirection(PlayerDirection direction) const
{
    switch (direction)
    {
        case PlayerDirection::SOUTH:       return "StandSouth";
        case PlayerDirection::EAST:        return "StandEast";
        case PlayerDirection::NORTH:       return "StandNorth";
        case PlayerDirection::WEST:        return "StandWest";
        case PlayerDirection::NORTH_EAST:  return "StandEast";
        case PlayerDirection::NORTH_WEST:  return "StandWest";
        case PlayerDirection::SOUTH_EAST:  return "StandEast";
        case PlayerDirection::SOUTH_WEST:  return "StandWest";
        default:                           return "StandSouth";
    }
}

PlayerController::PlayerDirection PlayerController::GetDirectionFromVector(const glm::vec2& direction) const
{
    if (direction == glm::vec2(0.0f)) return PlayerDirection::NONE;

    glm::vec2 normalized = glm::normalize(direction);

    if (normalized.x > 0.0f && normalized.y > 0.0f) return PlayerDirection::NORTH_EAST;
    if (normalized.x > 0.0f && normalized.y < 0.0f) return PlayerDirection::SOUTH_EAST;
    if (normalized.x < 0.0f && normalized.y > 0.0f) return PlayerDirection::NORTH_WEST;
    if (normalized.x < 0.0f && normalized.y < 0.0f) return PlayerDirection::SOUTH_WEST;
    if (normalized.x > 0.0f) return PlayerDirection::EAST;
    if (normalized.x < 0.0f) return PlayerDirection::WEST;
    if (normalized.y > 0.0f) return PlayerDirection::NORTH;
    if (normalized.y < 0.0f) return PlayerDirection::SOUTH;

    return PlayerDirection::NONE;
}

std::string PlayerController::GetAttackAnimationForDirection(PlayerDirection direction) const
{
    switch (direction)
    {
        case PlayerDirection::SOUTH:       return "AttackSouth";
        case PlayerDirection::EAST:        return "AttackEast";
        case PlayerDirection::NORTH:       return "AttackNorth";
        case PlayerDirection::WEST:        return "AttackWest";
        case PlayerDirection::NORTH_EAST:  return "AttackEast";
        case PlayerDirection::NORTH_WEST:  return "AttackWest";
        case PlayerDirection::SOUTH_EAST:  return "AttackEast";
        case PlayerDirection::SOUTH_WEST:  return "AttackWest";
        default:                           return "AttackSouth";
    }
}

void PlayerController::RegenerateStamina(float delta)
{
    if (m_stamina < m_maxStamina && m_staminaRegenTimer.Elapsed() >= m_staminaRegenDelay)
    {
        m_stamina += m_staminaRegenRate * delta;
        m_stamina = std::min(m_stamina, m_maxStamina); // Clamp stamina to max limit
    }
}

void PlayerController::StartAttack()
{
    m_isAttacking = true;
    m_action = PlayerAction::ATTACKING;
    m_attackTimer.Restart();
    m_animationFinished = false;

    std::string attackAnimation = GetAttackAnimationForDirection(m_lastDirectionEnum);

    m_pAnimComponent->SetTexture("data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f));
    m_pAnimComponent->SetAnimation(attackAnimation);

    m_currentAnimation = attackAnimation;
}

void PlayerController::UpdateAttackState(float delta)
{
    if (m_pAnimComponent->IsAnimationComplete()) m_animationFinished = true;

    if (m_animationFinished)
    {
        m_isAttacking = false;
        m_action = glm::length(m_pVelocity->GetVelocity()) < 0.01f ? PlayerAction::NONE : PlayerAction::WALKING;

        m_pAnimComponent->SetTexture("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f));
        m_currentAnimation = "";
        SetAnimationBasedOnState();
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
            }
        }
    }
}

void PlayerController::StartRoll()
{
    m_isRolling = true;
    m_rollTimer = m_rollDuration;
    m_action = PlayerAction::ROLLING;
    m_stamina -= 25.0f;
    m_staminaRegenTimer.Restart();
}

void PlayerController::EndRoll()
{
    m_isRolling = false;
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    m_action = PlayerAction::NONE;
}

void PlayerController::StartJump()
{
    m_isJumping = true;
    m_jumpTimer = m_jumpHeight / m_jumpSpeed;
    m_action = PlayerAction::JUMPING;
    m_pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed));
}

void PlayerController::EndJump()
{
    m_isJumping = false;
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    m_action = PlayerAction::NONE;
}

std::ostream& operator<<(std::ostream& os, const PlayerController::PlayerDirection& direction)
{
    switch (direction)
    {
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

void PlayerController::Render()
{
    if (!m_pTransform) return;

    glm::vec2 worldPos = m_pTransform->GetGlobalPosition();

    // Get the active camera and convert world position to screen position
    wolf::Camera2D* camera = GetGameObject()->GetScene().GetActiveCamera();
    if (!camera) return;
    glm::vec4 worldPos4(worldPos.x, worldPos.y, 0.0f, 1.0f);
    glm::vec4 clipSpacePos = camera->GetMatrix() * worldPos4;
    glm::vec3 ndcSpacePos = clipSpacePos / clipSpacePos.w;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    glm::vec2 screenPos = glm::vec2(
        (ndcSpacePos.x * 0.5f + 0.5f) * displaySize.x,
        (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * displaySize.y
    );
    screenPos.y -= 60.0f;

    // Draw stamina bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x - 25.0f, screenPos.y));
    ImGui::SetNextWindowSize(ImVec2(50.0f, 10.0f));
    ImGui::Begin("##StaminaBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::ProgressBar(m_stamina / m_maxStamina, ImVec2(-1, 0)); // Full width, default height
    ImGui::End();

    // Draw health bar below the stamina bar
    auto* healthComponent = GetGameObject()->GetComponent<HealthComponent>();
    if (healthComponent)
    {
        screenPos.y += 15.0f;  // Position health bar below stamina bar
        ImGui::SetNextWindowPos(ImVec2(screenPos.x - 25.0f, screenPos.y));
        ImGui::SetNextWindowSize(ImVec2(50.0f, 10.0f));
        ImGui::Begin("##HealthBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color for health bar
        ImGui::ProgressBar(healthComponent->GetHealth() / healthComponent->GetMaxHealth(), ImVec2(-1, 0));
        ImGui::PopStyleColor();
        ImGui::End();
    }
}