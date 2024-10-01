#include "PlayerController.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "HurtboxComponent.h"
#include "HitboxComponent.h"
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
        // Use the same component for all animations
        m_pAnimComponent = &pGameObject->AddComponent<AnimatedSprite2D>("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 12.0f);
    }

    InitializeAnimations();
}

void PlayerController::InitializeAnimations()
{
    if (!m_pAnimComponent) return;

    // Walk Animations
    m_pAnimComponent->AddAnimation("WalkSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, true);
    m_pAnimComponent->AddAnimation("WalkEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, true);
    m_pAnimComponent->AddAnimation("WalkNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, true);
    m_pAnimComponent->AddAnimation("WalkWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, true);
     m_pAnimComponent->SetOriginToCenterOfFrame();

    // Idle Animations
    m_pAnimComponent->AddAnimation("StandSouth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 1, false);
    m_pAnimComponent->AddAnimation("StandEast", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 9, false);
    m_pAnimComponent->AddAnimation("StandNorth", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 17, false);
    m_pAnimComponent->AddAnimation("StandWest", "data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 25, false);
     m_pAnimComponent->SetOriginToCenterOfFrame();

    // Attack Animations
    m_pAnimComponent->AddAnimation("AttackSouth", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 1, 8, false);
    m_pAnimComponent->AddAnimation("AttackEast", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 9, 16, false);
    m_pAnimComponent->AddAnimation("AttackNorth", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 17, 24, false);
    m_pAnimComponent->AddAnimation("AttackWest", "data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f), 25, 32, false);
     m_pAnimComponent->SetOriginToCenterOfFrame();

    m_pAnimComponent->SetAnimation("StandSouth");
}



void PlayerController::Update(float delta)
{
    auto* pGameObject = GetGameObject();
    if (pGameObject)
    {
        m_pTransform = m_pTransform ? m_pTransform : pGameObject->GetComponent<wolf::Transform2D>();
        m_pVelocity = m_pVelocity ? m_pVelocity : pGameObject->GetComponent<VelocityComponent>();

        if (!m_pAnimComponent)
        {
            LateInitialize();
        }
    }

    if (!m_pTransform || !m_pVelocity || !m_pAnimComponent)
    {
        std::cerr << "Error: Missing essential components in PlayerController!" << std::endl;
        return;
    }

    HandleMovement(delta);
    HandleRolling(delta);
    HandleJumping(delta);
    HandleAttacking(delta);
}

void PlayerController::SetManagers(HitboxManager* pHitboxManager, HurtboxManager* pHurtboxManager)
{
    m_pHitboxManager = pHitboxManager;
    m_pHurtboxManager = pHurtboxManager;
}


// Handle attacking input (Mouse1)
void PlayerController::HandleAttacking(float delta)
{
    if (wolf::Input::IsLMBJustDown())
    {
        m_isAttacking = true;
        m_action = PlayerAction::ATTACKING;  // Set state to attacking

        m_attackTimer.Restart();  // Restart attack animation timer

        // Set the attack animation based on direction and play it
        m_pAnimComponent->SetTexture("data/textures/TheseusSword-Sheet.png", glm::vec2(32.0f, 32.0f));
        switch (m_lastDirectionEnum)
        {
            case PlayerDirection::SOUTH:
                m_pAnimComponent->SetAnimation("AttackSouth");
                break;
            case PlayerDirection::EAST:
                m_pAnimComponent->SetAnimation("AttackEast");
                break;
            case PlayerDirection::NORTH:
                m_pAnimComponent->SetAnimation("AttackNorth");
                break;
            case PlayerDirection::WEST:
                m_pAnimComponent->SetAnimation("AttackWest");
                break;
            default:
                m_pAnimComponent->SetAnimation("AttackSouth");
                break;
        }

        m_pAnimComponent->SetOriginToCenterOfFrame();
        // m_pAnimComponent->SetVisible(true);

        ApplyDamageToEnemy();  // Deal damage
    }

    // Check if attack animation is complete
    if (m_attackTimer.Elapsed() >= m_attackCooldown)
    {
        m_isAttacking = false;  // Reset attacking flag

        // Restore to walk or idle animation based on velocity
        m_pAnimComponent->SetTexture("data/textures/TheseusWalk-Sheet.png", glm::vec2(32.0f, 32.0f));
        
        if (glm::length(m_pVelocity->GetVelocity()) < 0.01f)  // If standing still
        {
            m_action = PlayerAction::NONE;  // Set state to idle

            // Set idle animation based on last direction
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH:
                    m_pAnimComponent->SetAnimation("StandSouth");
                    break;
                case PlayerDirection::EAST:
                    m_pAnimComponent->SetAnimation("StandEast");
                    break;
                case PlayerDirection::NORTH:
                    m_pAnimComponent->SetAnimation("StandNorth");
                    break;
                case PlayerDirection::WEST:
                    m_pAnimComponent->SetAnimation("StandWest");
                    break;
                default:
                    m_pAnimComponent->SetAnimation("StandSouth");
                    break;
            }
        }
        else  // If moving
        {
            m_action = PlayerAction::WALKING;  // Set state to walking

            // Set walking animation based on last direction
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH:
                    m_pAnimComponent->SetAnimation("WalkSouth");
                    break;
                case PlayerDirection::EAST:
                    m_pAnimComponent->SetAnimation("WalkEast");
                    break;
                case PlayerDirection::NORTH:
                    m_pAnimComponent->SetAnimation("WalkNorth");
                    break;
                case PlayerDirection::WEST:
                    m_pAnimComponent->SetAnimation("WalkWest");
                    break;
                default:
                    m_pAnimComponent->SetAnimation("WalkSouth");
                    break;
            }
        }

        m_pAnimComponent->SetOriginToCenterOfFrame();
        // m_pAnimComponent->SetVisible(true);
    }
}
// Apply damage to the enemy if in range
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

// Handle movement input (WASD)
void PlayerController::HandleMovement(float delta)
{
    // Skip movement handling during rolling or if attacking is blocking
    if (m_action == PlayerAction::ROLLING) return;

    glm::vec2 direction = glm::vec2(0.0f);

    // Check movement input and set direction vector accordingly
    if (wolf::Input::IsKeyDown(GLFW_KEY_W)) direction.y += 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_S)) direction.y -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_A)) direction.x -= 1.0f;
    if (wolf::Input::IsKeyDown(GLFW_KEY_D)) direction.x += 1.0f;

    // Normalize the direction vector to ensure consistent speed
    if (direction != glm::vec2(0.0f))
    {
        direction = glm::normalize(direction);
        m_lastDirectionEnum = GetDirectionFromVector(direction);  // Update last direction enum

        // Update the player's velocity based on the direction and speed
        m_pVelocity->SetVelocity(direction * m_moveSpeed);

        // Only set walking animation if not attacking
        if (!m_isAttacking)
        {
            m_action = PlayerAction::WALKING;

            // Update walking animation based on direction
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH:
                    m_pAnimComponent->SetAnimation("WalkSouth");
                    break;
                case PlayerDirection::EAST:
                    m_pAnimComponent->SetAnimation("WalkEast");
                    break;
                case PlayerDirection::NORTH:
                    m_pAnimComponent->SetAnimation("WalkNorth");
                    break;
                case PlayerDirection::WEST:
                    m_pAnimComponent->SetAnimation("WalkWest");
                    break;
                default:
                    m_pAnimComponent->SetAnimation("WalkSouth");
                    break;
            }

            m_pAnimComponent->SetOriginToCenterOfFrame();
            // m_pAnimComponent->SetVisible(true);
        }
    }
    else
    {
        // If there's no movement, set velocity to zero
        m_pVelocity->SetVelocity(glm::vec2(0.0f));

        if (!m_isAttacking)
        {
            // Set state to NONE (idle) if not attacking
            m_action = PlayerAction::NONE;

            // Set standing animation based on the last known direction
            switch (m_lastDirectionEnum)
            {
                case PlayerDirection::SOUTH:
                    m_pAnimComponent->SetAnimation("StandSouth");
                    break;
                case PlayerDirection::EAST:
                    m_pAnimComponent->SetAnimation("StandEast");
                    break;
                case PlayerDirection::NORTH:
                    m_pAnimComponent->SetAnimation("StandNorth");
                    break;
                case PlayerDirection::WEST:
                    m_pAnimComponent->SetAnimation("StandWest");
                    break;
                default:
                    m_pAnimComponent->SetAnimation("StandSouth");
                    break;
            }

            m_pAnimComponent->SetOriginToCenterOfFrame();
            // m_pAnimComponent->SetVisible(true);
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

PlayerController::PlayerDirection PlayerController::GetDirectionFromVector(const glm::vec2& direction) const
{
    // Determine and return the correct enum based on direction
    if (direction.x == 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH;
    if (direction.x > 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH_EAST;
    if (direction.x > 0.0f && direction.y == 0.0f) return PlayerDirection::EAST;
    if (direction.x > 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH_EAST;
    if (direction.x == 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH;
    if (direction.x < 0.0f && direction.y < 0.0f) return PlayerDirection::SOUTH_WEST;
    if (direction.x < 0.0f && direction.y == 0.0f) return PlayerDirection::WEST;
    if (direction.x < 0.0f && direction.y > 0.0f) return PlayerDirection::NORTH_WEST;

    return PlayerDirection::NONE;  // Default case
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

        // Draw the health bar
        ImGui::ProgressBar(healthPercent, ImVec2(-1, 0)); // Full width, default height

        ImGui::End();
    }
}