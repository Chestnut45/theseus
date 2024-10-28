#include "PlayerController.h"
#include "VelocityComponent.h"
#include "HealthComponent.h"
#include "ColliderComponent.h"
#include "MinitaurController.h"
#include "HarpyController.h"
#include <W_Input.h>
#include <W_Logging.h>

//-----------------------------------------------------------------------------
// File:            PlayerController.cpp
// Original Author: Youssef Ashraf
// ver 2.0: Optimized and restructured for readability and performance.
//-----------------------------------------------------------------------------

float PlayerController::s_aAttackCooldown[(int)WeaponType::BOW + 1] = {0.25f, 1.0f, 0.5f};

PlayerController::PlayerController() = default;

PlayerController::~PlayerController() {
    wolf::EventManager::RemoveListener<WeaponEquippedEvent, PlayerController, &PlayerController::HandleWeaponEquippedEvent>(*this);
    wolf::EventManager::RemoveListener<ArmourEquippedEvent, PlayerController, &PlayerController::HandleArmourEquippedEvent>(*this);
}

void PlayerController::SetAnimationComponent(AnimatedSprite2D* animComponent)
{
    m_pAnimComponent = animComponent;
}

void PlayerController::SetColliderManager(ColliderManager* pColliderManager)
{
    m_pColliderManager = pColliderManager;
}

ColliderManager* PlayerController::GetColliderManager() const
{
    return m_pColliderManager;
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

    InitializeAnimations();

    // !-- Aurora added this --!
    wolf::EventManager::AddListener<WeaponEquippedEvent, PlayerController, &PlayerController::HandleWeaponEquippedEvent>(*this);
    wolf::EventManager::AddListener<ArmourEquippedEvent, PlayerController, &PlayerController::HandleArmourEquippedEvent>(*this);
}

// Add and initialize animations for the player character
void PlayerController::InitializeAnimations()
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from player...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &pGameObject->AddComponent<AnimatedSprite2D>("data/player_anim_init.yaml");
}

// Main update loop for the player controller
void PlayerController::Update(float delta)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject) return;

    // Retrieve the active camera through the game's scene using the game object
    auto* pCamera = pGameObject->GetScene().GetActiveCamera();
    if (pCamera)
    {
        float prevZoom = pCamera->GetZoom();
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_EQUAL)) pCamera->SetZoom(prevZoom * 2);
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_MINUS)) pCamera->SetZoom(prevZoom * 0.5f);
    }

    if (!m_pTransform) m_pTransform = GetGameObject()->GetComponent<wolf::Transform2D>();
    if (!m_pVelocity) m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pAnimComponent) LateInitialize();

    if (!m_pTransform || !m_pVelocity || !m_pAnimComponent)
    {
        wolf::Error("PlayerController missing essential components!");
        return;
    }
    
    // Debug speed modifier hotkeys
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_PAGE_DOWN))
    {
        m_moveSpeed *= 0.5f;
        m_rollSpeed *= 0.5f;
        m_inventoryMoveSpeed *= 0.5f;
       
    }
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_PAGE_UP))
    {
        m_moveSpeed *= 2;
        m_rollSpeed *= 2;
        m_inventoryMoveSpeed *= 2;


    }
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_HOME))
    {
        m_moveSpeed = 200.0f;
        m_rollSpeed = 400.0f;
        m_inventoryMoveSpeed = 200.0f;

    }

    HandlePlayerInput(delta);
    RegenerateStamina(delta);

    // Call SetAnimationBasedOnState() only if the action or direction has changed
    if (m_action != m_previousAction || m_lastDirectionEnum != m_previousDirection)
    {
        SetAnimationBasedOnState();
        m_previousAction = m_action;
        m_previousDirection = m_lastDirectionEnum;
    }
}


// Handle all player inputs and manage states accordingly
void PlayerController::HandlePlayerInput(float delta) 
{
    auto* playerInventory = GetGameObject()->GetComponent<PlayerInventoryComponent>();

    // Check for Left Alt key (hold) to manage the inventory state
    if (wolf::Input::IsKeyDown(GLFW_KEY_LEFT_ALT)) 
    {
        if (m_action != PlayerAction::IN_INVENTORY) 
        {
            playerInventory->Open();
            m_action = PlayerAction::IN_INVENTORY;
        }
    } 
    else if (wolf::Input::IsKeyReleased(GLFW_KEY_LEFT_ALT)) 
    {
        playerInventory->Close();
        m_action = PlayerAction::NONE;
    }

    // IN_INVENTORY state when inventory is toggled with 0 (handled in PlayState)
    if (playerInventory && playerInventory->IsOpen()) 
    {
        m_action = PlayerAction::IN_INVENTORY;
    }
    else if (m_action == PlayerAction::IN_INVENTORY) 
    {
        m_action = PlayerAction::NONE;
    }

    // Skip input handling for attacks when in inventory
    if (m_action != PlayerAction::IN_INVENTORY)
    {
        HandleAttacking(delta);
        HandleRolling(delta);
        HandleJumping(delta);
    }

    // Process movement input regardless of inventory state
    HandleMovement(delta);
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
        m_walkSoundTimer.Reset();
        return;
    }

    // Play walking sound effect
    if (!m_walkSoundTimer.IsRunning()) m_walkSoundTimer.Start();
    if (m_walkSoundTimer.Elapsed() > m_walkSoundInterval)
    {
        wolf::Audio::Play("data/sounds/walk.wav");
        m_walkSoundTimer.Restart();
    }

    direction = glm::normalize(direction);
    m_lastDirectionEnum = GetDirectionFromVector(direction);
    float currentSpeed = (m_action == PlayerAction::IN_INVENTORY) ? m_inventoryMoveSpeed : m_moveSpeed;
    m_pVelocity->SetVelocity(direction * currentSpeed);

    if (!m_isRolling && !m_isJumping) m_action = PlayerAction::WALKING;
}

// Manage attack state and animation transitions
void PlayerController::HandleAttacking(float delta)
{
    if (m_action == PlayerAction::IN_INVENTORY) {
        // Disable attacking while in inventory
        return;
    }

    // Start the attack if the left mouse button is pressed and the player is not currently attacking.
    if (wolf::Input::IsLMBJustDown() && !m_isAttacking)
    {
        StartAttack();
    }

    // Check if enough time has elapsed since the last attack to allow for damage application.
    if (m_attackTimer.Elapsed() >= s_aAttackCooldown[(int)m_eCurrentWeapon] && m_isAttacking)
    {
        ApplyDamageToEnemy(); // Apply damage if there's a collision with an enemy.
        m_attackTimer.Restart(); // Restart the timer for future attacks.
    }

    // Update the attack state and manage transitions.
    if (m_isAttacking)
    {
        UpdateAttackState(delta);
    }
}
// Handle rolling logic based on player input and stamina
void PlayerController::HandleRolling(float delta)
{
    // Prevent rolling if the player is in the inventory state
    if (m_action == PlayerAction::IN_INVENTORY) return;

    if (m_isRolling)
    {
        m_rollTimer -= delta;
        if (m_rollTimer <= 0.0f) EndRoll();
        return;
    }

    // Only start roll if a direction is being held and sufficient stamina is available
    glm::vec2 direction(0.0f);
    direction.y += wolf::Input::IsKeyDown(GLFW_KEY_W) ? 1.0f : 0.0f;
    direction.y -= wolf::Input::IsKeyDown(GLFW_KEY_S) ? 1.0f : 0.0f;
    direction.x -= wolf::Input::IsKeyDown(GLFW_KEY_A) ? 1.0f : 0.0f;
    direction.x += wolf::Input::IsKeyDown(GLFW_KEY_D) ? 1.0f : 0.0f;

    if (direction != glm::vec2(0.0f) && wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE) && m_stamina >= 15.0f)
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
    // Skip if the player is performing an action that overrides animations like attacking, rolling, jumping, or inventory management.
    if (m_isAttacking || m_isRolling || m_isJumping || m_action == PlayerAction::IN_INVENTORY) return;

    std::string animationName;

    // Determine the correct animation based on state and direction.
    switch (m_action)
    {
        case PlayerAction::WALKING:
            animationName = GetWalkAnimationForDirection(m_lastDirectionEnum);
            break;

        case PlayerAction::NONE:  // Idle state.
            animationName = GetIdleAnimationForDirection(m_lastDirectionEnum);
            break;

        default:
            return;  // No need to change animation for other states.
    }

    // Check if the desired animation is different from the currently playing one.
    if (!animationName.empty() && animationName != m_currentAnimation)
    {
        // Set the new animation.
        m_pAnimComponent->SetAnimation(animationName);

        // Update the current animation name.
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
        case PlayerDirection::SOUTH:       return "SwordAttackSouth";
        case PlayerDirection::EAST:        return "SwordAttackEast";
        case PlayerDirection::NORTH:       return "SwordAttackNorth";
        case PlayerDirection::WEST:        return "SwordAttackWest";
        case PlayerDirection::NORTH_EAST:  return "SwordAttackEast";
        case PlayerDirection::NORTH_WEST:  return "SwordAttackWest";
        case PlayerDirection::SOUTH_EAST:  return "SwordAttackEast";
        case PlayerDirection::SOUTH_WEST:  return "SwordAttackWest";
        default:                           return "SwordAttackSouth";
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
    // Check if the player is not already attacking to prevent re-triggering attacks mid-animation.
    if (!m_isAttacking)
    {
        m_isAttacking = true;
        m_animationFinished = false;
        m_hasAppliedDamage = false;

        // Set the player action to attacking and reset attack-related timers.
        m_action = PlayerAction::ATTACKING;
        m_attackTimer.Restart();

        // Choose the correct animation based on the player's direction.
        std::string attackAnimation = GetAttackAnimationForDirection(m_lastDirectionEnum);

        // Set the attacking animation.
        m_pAnimComponent->SetAnimation(attackAnimation);

        // Store the current animation to handle transitions later.
        m_currentAnimation = attackAnimation;

        // Attack
        glm::vec2 playerDirection;
        switch (this->m_lastDirectionEnum)
        {
            case PlayerDirection::NORTH:       
                playerDirection = glm::normalize(glm::vec2(0.0f, 1.0f));
                break;

            case PlayerDirection::NORTH_EAST:  
                playerDirection = glm::normalize(glm::vec2(1.0f, 1.0f));
                break;

            case PlayerDirection::EAST:        
                playerDirection = glm::normalize(glm::vec2(1.0f, 0.0f));
                break;

            case PlayerDirection::SOUTH_EAST:  
                playerDirection = glm::normalize(glm::vec2(1.0f, -1.0f));
                break;

            case PlayerDirection::SOUTH:       
                playerDirection = glm::normalize(glm::vec2(0.0f, -1.0f));
                break;

            case PlayerDirection::SOUTH_WEST:  
                playerDirection = glm::normalize(glm::vec2(-1.0f, -1.0f));
                break;
            
            case PlayerDirection::WEST:        
                playerDirection = glm::normalize(glm::vec2(-1.0f, 0.0f));
                break;

            case PlayerDirection::NORTH_WEST:  
                playerDirection = glm::normalize(glm::vec2(-1.0f, 1.0f));
                break;

            default:
                playerDirection = glm::vec2(0.0f, 0.0f);
                break;
        }
        glm::vec2 playerVelocity = glm::vec2(0.0f);


        switch(this->m_eCurrentWeapon)
        {
            case WeaponType::BOW:
            {
                auto& scene = this->GetGameObject()->GetScene();
                auto& projectile = scene.CreateObject2D();
                
                auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
                projectileSprite.SetOriginToCenterOfTexture();
                
                auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
                projectileCollider.SetDamage(10.0f);
                projectileCollider.AddColliderBox(glm::vec2(32.0f, 32.0f), glm::vec2(-16.0f, -16.0f));

                
                auto& projectileVelocity = projectile.AddComponent<VelocityComponent>();

                projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(1));
                projectile.GetComponent<wolf::Transform2D>()->SetPosition(glm::vec2(this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition()));

                VelocityComponent* playerVelocityComponent = this->GetGameObject()->GetComponent<VelocityComponent>();

                if(playerVelocityComponent != nullptr)
                {
                    playerVelocity = playerVelocityComponent->GetVelocity();
                }
                else
                {
                    playerVelocity = glm::vec2(0.0f, 0.0f);
                }
                projectileVelocity.SetVelocity(playerDirection * 256.0f + playerVelocity);
            
                break;
            }
        }
    }
}

void PlayerController::UpdateAttackState(float delta)
{
    // Check if the animation has finished playing all its frames
    if (m_pAnimComponent->IsAnimationFinished())
    {
        m_animationFinished = true;
    }

    // If the animation has finished, transition out of the attacking state
    if (m_animationFinished)
    {

        // Reset all attack-related flags
        m_isAttacking = false;
        m_animationFinished = false;
        m_hasAppliedDamage = false;

        // Determine the next action based on the player's velocity
        if (glm::length(m_pVelocity->GetVelocity()) < 0.01f)
        {
            m_action = PlayerAction::NONE; // Set to idle state
        }
        else
        {
            m_action = PlayerAction::WALKING; // Set to walking state
        }

        // Clear the current animation and set a new one based on the updated state
        m_currentAnimation = "";
        SetAnimationBasedOnState();
    }
}

void PlayerController::ApplyDamageToEnemy()
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject || !m_pTransform) return;

    // Iterate through all Minitaurs in the scene (MinitaurController)
    for (auto&& [entity, minitaurController] : GetGameObject()->GetScene().Each<MinitaurController>())
    {
        // Get the transform of the Minitaur
        auto* minitaurTransform = minitaurController.GetGameObject()->GetComponent<wolf::Transform2D>();
        auto* minitaurHealth = minitaurController.GetGameObject()->GetComponent<HealthComponent>();

        // Ensure the Minitaur has a HealthComponent and a Transform
        if (!minitaurTransform || !minitaurHealth) continue;

        // Calculate the distance between the player and the Minitaur
        const glm::vec2 playerPosition = m_pTransform->GetGlobalPosition();
        const glm::vec2 minitaurPosition = minitaurTransform->GetGlobalPosition();
        const float distanceToMinitaur = glm::length(playerPosition - minitaurPosition);

        // Check if the Minitaur is within attack range
        if (distanceToMinitaur <= m_attackRange)
        {
            // Apply damage to the Minitaur
            minitaurHealth->Damage(m_attackDamage);
            std::cout << "Player attacked Minitaur! Damage: " << m_attackDamage << std::endl;
            std::cout << "Minitaur Health: " << minitaurHealth->GetHealth() << std::endl;

            wolf::Audio::Play("data/sounds/hit.wav");

            // Optionally, break here if you're only targeting one Minitaur at a time
            // break;
        }
    }

    // Iterate through all Harpies in the scene (HarpyController)
    for (auto&& [entity, harpyController] : GetGameObject()->GetScene().Each<HarpyController>())
    {
        // Get the transform of the Harpy
        auto* harpyTransform = harpyController.GetGameObject()->GetComponent<wolf::Transform2D>();
        auto* harpyHealth = harpyController.GetGameObject()->GetComponent<HealthComponent>();

        // Ensure the Harpy has a HealthComponent and a Transform
        if (!harpyTransform || !harpyHealth) continue;

        // Calculate the distance between the player and the Harpy
        const glm::vec2 playerPosition = m_pTransform->GetGlobalPosition();
        const glm::vec2 harpyPosition = harpyTransform->GetGlobalPosition();
        const float distanceToHarpy = glm::length(playerPosition - harpyPosition);

        // Check if the Harpy is within attack range
        if (distanceToHarpy <= m_attackRange)
        {
            // Apply damage to the Harpy
            harpyHealth->Damage(m_attackDamage);
            std::cout << "Player attacked Harpy! Damage: " << m_attackDamage << std::endl;
            std::cout << "Harpy Health: " << harpyHealth->GetHealth() << std::endl;

            // Optionally, break here if you're only targeting one Harpy at a time
            // break;
        }
    }
}
void PlayerController::StartRoll()
{
    m_isRolling = true;
    m_rollTimer = m_rollDuration;
    glm::vec2 rollDirection = m_pVelocity->GetVelocity(); // Get current movement direction
    if (glm::length(rollDirection) > 0.0f)
    {
        rollDirection = glm::normalize(rollDirection) * m_rollSpeed; // Set velocity based on roll speed
        m_pVelocity->SetVelocity(rollDirection);
    }
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

    float barWidth = 180.0f;
    float barHeight = 18.0f;
    float verticalOffset = 10.0f;  // Offset between health and stamina bars

    // Position both bars at the top-center of the screen
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 basePos = ImVec2(displaySize.x / 2.0f - barWidth / 2.0f, 20.0f);

    // Push ImGui style variables for a more polished and "arty" look
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);          // Rounded corners
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);           // Rounded frame corners
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f)); // Inner padding
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.8f)); // Semi-transparent background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));   // Border color
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.1f, 0.1f, 0.1f, 0.5f)); // Shadow effect

    // Render the health bar at the top-center of the screen
    auto* healthComponent = GetGameObject()->GetComponent<HealthComponent>();
    if (healthComponent)
    {
        ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y)); // Position for health bar
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##HealthBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Deep red health color
        ImGui::ProgressBar(healthComponent->GetHealth() / healthComponent->GetMaxHealth(), ImVec2(-1, barHeight));
        ImGui::PopStyleColor(); // Pop color for health bar
        ImGui::End();
    }

    // Move the position down for the stamina bar
    basePos.y += (barHeight + verticalOffset);

    // Render the stamina bar below the health bar
    ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y)); // Position for stamina bar
    ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
    ImGui::Begin("##StaminaBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green stamina color
    ImGui::ProgressBar(m_stamina / m_maxStamina, ImVec2(-1, barHeight)); // Full width, defined height
    ImGui::PopStyleColor(); // Pop color for stamina bar
    ImGui::End();

    // Pop ImGui style variables and colors
    ImGui::PopStyleVar(3); // Pop style variables (WindowRounding, FrameRounding, and FramePadding)
    ImGui::PopStyleColor(3); // Pop style colors (WindowBg, Border, and BorderShadow)
}

// !-- Aurora added this --!
void PlayerController::HandleWeaponEquippedEvent(const WeaponEquippedEvent& p_event) {
    printf("The player equipped a %s!\n", p_event.pWeapon->GetName().c_str());
}

void PlayerController::HandleArmourEquippedEvent(const ArmourEquippedEvent& p_event) {
    printf("The player equipped a %s!\n", p_event.pArmour->GetName().c_str());
}