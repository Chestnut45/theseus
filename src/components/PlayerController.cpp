
#include "AttackDamageComponent.h"
#include "ColliderComponent.h"
#include "HealthComponent.h"
#include "VelocityComponent.h"
#include "TimedDestroyerComponent.h"
#include "HarpyController.h"
#include "MinitaurController.h"
#include "PlayerController.h"

#include "../inventory/ItemCreator.h"

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
    wolf::EventManager::RemoveListener<WeaponUnequippedEvent, PlayerController, &PlayerController::HandleWeaponUnequippedEvent>(*this);
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

void PlayerController::SetAction(PlayerAction action)
{
    // If transitioning to THROWING or PICKING_UP state, reset any active attack
    if ((action == PlayerAction::THROWING || action == PlayerAction::PICKING_UP) && m_isAttacking)
    {
        m_isAttacking = false;
        m_action = PlayerAction::NONE; // Reset to NONE to avoid conflict
    }

    m_action = action;
}

void PlayerController::SetHoldingObject(bool isHolding) {
    m_isHoldingObject = isHolding;
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

    this->m_pDefaultWeapon = dynamic_cast<WeaponItem*>(ItemCreator::CreateItem("Dull Blade"));
    this->m_pCurrentWeapon = this->m_pDefaultWeapon;

    InitializeAnimations();

    // !-- Aurora added this --!
    wolf::EventManager::AddListener<WeaponEquippedEvent, PlayerController, &PlayerController::HandleWeaponEquippedEvent>(*this);
    wolf::EventManager::AddListener<WeaponUnequippedEvent, PlayerController, &PlayerController::HandleWeaponUnequippedEvent>(*this);
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
        m_inventoryMoveSpeed = 100.0f;
    }

    HandlePlayerInput(delta);
    RegenerateStamina(delta);

    // Call SetAnimationBasedOnState() only if the action or direction has changed
    if (m_action != m_previousAction || m_lastMoveDirectionEnum != m_previousDirection)
    {
        SetAnimationBasedOnState();
        m_previousAction = m_action;
        m_previousDirection = m_lastMoveDirectionEnum;
    }
}


void PlayerController::HandlePlayerInput(float delta)
{
    auto* playerInventory = GetGameObject()->GetComponent<PlayerInventoryComponent>();

    // Handle inventory management with left alt
    if (wolf::Input::IsKeyDown(GLFW_KEY_LEFT_ALT)) {
        if (m_action != PlayerAction::IN_INVENTORY) {
            playerInventory->Open();
            SetAction(PlayerAction::IN_INVENTORY);
        }
    } else if (wolf::Input::IsKeyReleased(GLFW_KEY_LEFT_ALT)) {
        playerInventory->Close();
        SetAction(PlayerAction::NONE);
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

    // Handle pick up and drop actions
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
        PickUpObject();
    }
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_Q)) {
        DropObject();
    }

    // If holding an object, handle throw/drop actions
    if (m_isHoldingObject) {
        HandleThrowing(delta);  // Throw if needed
        HandleMovement(delta);  // Continue to allow movement
        return;  // Skip attack or other actions while holding an object
    }

    // Handle regular player actions
    switch (m_action) {
        case PlayerAction::IN_INVENTORY:
            HandleMovement(delta);  // Allow movement while in inventory
            break;
        case PlayerAction::PICKING_UP:
            SetAction(PlayerAction::NONE);  // Transition to NONE after picking up
            break;
        case PlayerAction::THROWING:
            HandleThrowing(delta);  // Handle throw logic
            HandleMovement(delta);
            break;
        default:
            HandleAttacking(delta);
            HandleRolling(delta);
            HandleJumping(delta);
            HandleMovement(delta);
            break;
    }
}

void PlayerController::PickUpObject() {
    // Attempt to pick up a nearby throwable object
    for (auto&& [entity, throwable] : GetGameObject()->GetScene().Each<ThrowableObjectComponent>()) {
        if (throwable.IsCloseToPlayer(150.0f)) {  // Check proximity
            throwable.PickUp();
            m_pHeldObject = &throwable;           // Store reference to the held object
            m_isHoldingObject = true;
            SetAction(PlayerAction::PICKING_UP);  // Temporary state while picking up
            // std::cout << "Picked up object!" << std::endl;
            return;
        }
    }
}

void PlayerController::DropObject() {
    if (m_isHoldingObject && m_pHeldObject) {
        m_pHeldObject->Drop();
        m_isHoldingObject = false;
        m_pHeldObject = nullptr;

        // Reset throw power variables
        m_throwPower = 0.0f;

        SetAction(PlayerAction::NONE);
        // std::cout << "Dropped object!" << std::endl;
    }
}


void PlayerController::HandleThrowing(float delta) {
    // Charge the throw power while holding the button
    if (wolf::Input::IsLMBHeld() && m_isHoldingObject) {
        m_throwPower += 75.0f * delta;
        m_throwPower = std::min(m_throwPower, m_maxThrowPower); // Cap to max throw power
        SetAction(PlayerAction::THROWING);
    }

    // Release to throw the object
    if (wolf::Input::IsLMBReleased() && m_isHoldingObject) {
        ThrowHeldObject();
        m_throwPower = 0.0f;  // Reset power after throwing
        m_isHoldingObject = false;
        SetAction(PlayerAction::NONE);  // Reset action after throwing
    }
}


void PlayerController::ThrowHeldObject() {
    if (!m_isHoldingObject || !m_pHeldObject) {
        // std::cout << "No object is being held to throw!" << std::endl;
        return;
    }

    // Determine throw direction based on player’s facing direction
    glm::vec2 throwDirection;
    switch (m_lastFaceDirectionEnum) {
        case PlayerDirection::NORTH:       throwDirection = glm::vec2(0.0f, 1.0f); break;
        case PlayerDirection::EAST:        throwDirection = glm::vec2(1.0f, 0.0f); break;
        case PlayerDirection::SOUTH:       throwDirection = glm::vec2(0.0f, -1.0f); break;
        case PlayerDirection::WEST:        throwDirection = glm::vec2(-1.0f, 0.0f); break;
        case PlayerDirection::NORTH_EAST:  throwDirection = glm::normalize(glm::vec2(1.0f, 1.0f)); break;
        case PlayerDirection::NORTH_WEST:  throwDirection = glm::normalize(glm::vec2(-1.0f, 1.0f)); break;
        case PlayerDirection::SOUTH_EAST:  throwDirection = glm::normalize(glm::vec2(1.0f, -1.0f)); break;
        case PlayerDirection::SOUTH_WEST:  throwDirection = glm::normalize(glm::vec2(-1.0f, -1.0f)); break;
        default:                           throwDirection = glm::vec2(1.0f, 0.0f); break; // Default to right
    }

    // Get player velocity
    VelocityComponent* playerVelocityComponent = GetGameObject()->GetComponent<VelocityComponent>();
    glm::vec2 playerVelocity = playerVelocityComponent ? playerVelocityComponent->GetVelocity() : glm::vec2(0.0f);

    // Set the object's velocity based on throw direction, throw power, and player's velocity
    if (auto* throwableVelocity = m_pHeldObject->GetGameObject()->GetComponent<VelocityComponent>()) {
        glm::vec2 finalVelocity = throwDirection * m_throwPower + playerVelocity;
        throwableVelocity->SetVelocity(finalVelocity);
        // std::cout << "[DEBUG] Object thrown with velocity: (" 
                //   << finalVelocity.x << ", " << finalVelocity.y << ")" << std::endl;
    }

    // Set the state of the held object to THROWN and reset holding variables
    m_pHeldObject->SetState(ThrowableState::THROWN);
    m_isHoldingObject = false;
    m_pHeldObject = nullptr;
    m_throwPower = 0.0f;  // Reset throw power after throw
    SetAction(PlayerAction::NONE);
}

// Handle player movement based on input
void PlayerController::HandleMovement(float delta)
{
    if (m_action == PlayerAction::ROLLING) return;  // Skip movement if rolling 

    glm::vec2 direction(0.0f);

    // Track and update currently held keys for smooth directional input
    direction.y += wolf::Input::IsKeyDown(GLFW_KEY_W) ? 1.0f : 0.0f;
    direction.y -= wolf::Input::IsKeyDown(GLFW_KEY_S) ? 1.0f : 0.0f;
    direction.x -= wolf::Input::IsKeyDown(GLFW_KEY_A) ? 1.0f : 0.0f;
    direction.x += wolf::Input::IsKeyDown(GLFW_KEY_D) ? 1.0f : 0.0f;

    if (glm::length(direction) == 0.0f) {
        if (!m_isAttacking && !m_isRolling) m_action = PlayerAction::NONE;
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        m_walkSoundTimer.Reset();
        return;
    }

    // Play walking sound effect
    if (!m_walkSoundTimer.IsRunning()) m_walkSoundTimer.Start();
    if (m_walkSoundTimer.Elapsed() > m_walkSoundInterval) {
        wolf::Audio::Play("data/sounds/walk.wav");
        m_walkSoundTimer.Restart();
    }

    direction = glm::normalize(direction);
    m_lastMoveDirectionEnum = GetDirectionFromVector(direction);
    float currentSpeed = (m_action == PlayerAction::IN_INVENTORY) ? m_inventoryMoveSpeed : m_moveSpeed;
    m_pVelocity->SetVelocity(direction * currentSpeed);

    if (!m_isRolling && !m_isJumping) m_action = PlayerAction::WALKING;
}

// Manage attack state and animation transitions
void PlayerController::HandleAttacking(float delta)
{
    // Disable attacking when in inventory, picking up, or holding a throwable object
    if (m_action == PlayerAction::IN_INVENTORY || m_action == PlayerAction::PICKING_UP || m_action == PlayerAction::THROWING) {
        return;
    }

    // Start the attack if the left mouse button is pressed and the player is not currently attacking.
    if (wolf::Input::IsLMBJustDown() && !m_isAttacking)
    {
        StartAttack();
    }

    // Check if enough time has elapsed since the last attack to allow for damage application.
    if (m_attackTimer.Elapsed() >= s_aAttackCooldown[(int)m_pCurrentWeapon->GetWeaponType()] && m_isAttacking)
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
            animationName = GetWalkAnimationForDirection(m_lastMoveDirectionEnum);
            break;

        case PlayerAction::NONE:  // Idle state.
            animationName = GetIdleAnimationForDirection(m_lastMoveDirectionEnum);
            break;

        default:
            return;  // No need to change animation for other states.
    }

    // Set facing direction
    m_lastFaceDirectionEnum = m_lastMoveDirectionEnum;

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
    std::string weaponType = "Sword";
    WeaponType type = this->m_pCurrentWeapon->GetWeaponType();
    if(type == WeaponType::BOW) weaponType = "Bow";
    else if(type == WeaponType::SPEAR) weaponType = "Spear";
    //------------------------//
    //                        //
    //  CHANGE TYPE TO SWORD  //
    //                        //
    //------------------------//
    else if(type == WeaponType::SWORD) weaponType = "Spear";

    switch (direction)
    {
        case PlayerDirection::SOUTH:       return weaponType + "AttackSouth";
        case PlayerDirection::EAST:        return weaponType + "AttackEast";
        case PlayerDirection::NORTH:       return weaponType + "AttackNorth";
        case PlayerDirection::WEST:        return weaponType + "AttackWest";
        case PlayerDirection::NORTH_EAST:  return weaponType + "AttackEast";
        case PlayerDirection::NORTH_WEST:  return weaponType + "AttackWest";
        case PlayerDirection::SOUTH_EAST:  return weaponType + "AttackEast";
        case PlayerDirection::SOUTH_WEST:  return weaponType + "AttackWest";
        default:                           return weaponType + "AttackSouth";
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
        std::string attackAnimation = GetAttackAnimationForDirection(m_lastMoveDirectionEnum);

        // Set the attacking animation.
        m_pAnimComponent->SetAnimation(attackAnimation);

        // Store the current animation to handle transitions later.
        m_currentAnimation = attackAnimation;
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
    // Attack
    auto* player = this->GetGameObject();
    if (!player || !m_pTransform) return;

    glm::vec2 playerScale = player->GetComponent<wolf::Transform2D>()->GetGlobalScale();
    VelocityComponent* playerVelocityComponent = player->GetComponent<VelocityComponent>();
    glm::vec2 playerVelocity = playerVelocityComponent == nullptr ? glm::vec2(0.0f) : playerVelocityComponent->GetVelocity();
    glm::vec2 playerDirection;
    glm::vec2 spawnOffset;

    switch (this->m_lastFaceDirectionEnum)
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

    switch(this->m_pCurrentWeapon->GetWeaponType())
    {
        // Spawn projectile for bow
        case WeaponType::BOW:
        {
            // Set data for projectile collider
            glm::vec2 projectileDimensions = glm::vec2(32.0f, 32.0f);
            glm::vec2 hurtboxOffset = glm::vec2(-16.0f, 16.0f);

            // Spawn projectile object & add components
            auto& scene = player->GetScene();
            auto& projectile = scene.CreateObject2D();

            auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>("data/textures/DebugSprites/debug_sprite.png");
            projectileSprite.SetOriginToCenterOfTexture();
            
            auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
            projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
            projectileCollider.SetIgnoreTag(player->GetID());

            auto& projectileADComponent = projectile.AddComponent<AttackDamageComponent>(m_pCurrentWeapon->GetDamage(), m_pColliderManager);

            spawnOffset.x = spawnOffset.x > 0.0f ? (spawnOffset.x + projectileDimensions.x * 0.5f) : ( spawnOffset.x < 0.0f ? (spawnOffset.x - projectileDimensions.x * 0.5f) : (spawnOffset.x));
            spawnOffset.y = spawnOffset.y > 0.0f ? (spawnOffset.y + projectileDimensions.y * 0.5f) : ( spawnOffset.y < 0.0f ? (spawnOffset.y - projectileDimensions.y * 0.5f) : (spawnOffset.y));
                        
            projectile.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
           
            auto& projectileVelocity = projectile.AddComponent<VelocityComponent>();
            projectileVelocity.SetVelocity(playerDirection * 256.0f + playerVelocity);
        
            break;
        }
        
        // Spawn melee collider for sword
        //------------------------//
        //                        //
        //  CHANGE TYPE TO SPEAR  //
        //                        //
        //------------------------//
        case WeaponType::SPEAR:
        {
            // Set & calculate data for melee collider
            glm::vec2 meleeDimensions = glm::vec2(12.0f, 12.0f);
            glm::vec2 offset = glm::vec2(0.0f, 0.0f);
             
            if(playerDirection.x != 0.0f)
            {
                offset.x = 9.0f;
                meleeDimensions.y *= 2.0f;
            }

            if(playerDirection.y != 0.0f)
            {
                offset.y = 12.0f;
                meleeDimensions.x *= 2.0f;
            }

            // Shift offset along player direction
            offset.x = playerDirection.x * offset.x;
            offset.y = playerDirection.y * offset.y;

            // Scale offset by player scale
            offset *= playerScale;

            auto& scene = player->GetScene();

            // Create melee object & add components
            auto& melee = scene.CreateObject2D();
            melee.GetComponent<wolf::Transform2D>()->SetScale(playerScale);

            auto& meleeCollider = melee.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
            meleeCollider.AddColliderBox(meleeDimensions, glm::vec2(-meleeDimensions.x * 0.5f, meleeDimensions.y * 0.5f - 0.5f));
            meleeCollider.SetIgnoreTag(player->GetID());

            auto& meleeADcomponent = melee.AddComponent<AttackDamageComponent>(m_pCurrentWeapon->GetDamage(), m_pColliderManager);
            
            melee.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(playerScale));
            melee.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition() + offset);
            auto& meleeTD = melee.AddComponent<TimedDestroyerComponent>(1,1);

            auto& meleeVelocity = melee.AddComponent<VelocityComponent>();
            meleeVelocity.SetVelocity(glm::vec2(0.0f, 0.0f));

            break;
        }

        // Spawn melee collider for spear
        //------------------------//
        //                        //
        //  CHANGE TYPE TO SPEAR  //
        //                        //
        //------------------------//
        case WeaponType::SWORD:
        {
            // Set & calculate data for melee collider
            glm::vec2 meleeDimensions = glm::vec2(12.0f, 12.0f);
            glm::vec2 offset = glm::vec2(0.0f, 0.0f);
            glm::vec2 colliderBoxOffset2 = glm::vec2(0.0f, 0.0f);
            
            if(playerDirection.x != 0.0f)
            {
                offset.x = 9.0f;
                meleeDimensions.y *= 2.5f;
                colliderBoxOffset2.x = playerDirection.x > 0.0f ? meleeDimensions.x : -meleeDimensions.x;
            }

            if(playerDirection.y != 0.0f)
            {
                offset.y = 13.0f;
                meleeDimensions.x *= 3.0f;
                colliderBoxOffset2.y = playerDirection.y > 0.0f ? meleeDimensions.y : -meleeDimensions.y;
            }

            bool isDiagonalAttack = (playerDirection.x != 0.0f) && (playerDirection.y != 0.0f);

            // Shift offset along player direction
            offset.x = playerDirection.x * offset.x;
            offset.y = playerDirection.y * offset.y;

            // Scale offset by player scale
            offset *= playerScale;

            auto& scene = player->GetScene();

            // Create melee object & add components
            auto& melee = scene.CreateObject2D();
            melee.GetComponent<wolf::Transform2D>()->SetScale(playerScale);

            auto& meleeCollider = melee.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
            meleeCollider.AddColliderBox(meleeDimensions, glm::vec2(-meleeDimensions.x * 0.5f, meleeDimensions.y * 0.5 - 0.5f));
            meleeCollider.AddColliderBox(meleeDimensions, glm::vec2(-meleeDimensions.x * 0.5f, meleeDimensions.y * 0.5 - 0.5f) + colliderBoxOffset2);
            meleeCollider.SetIgnoreTag(player->GetID());

            auto& meleeADcomponent = melee.AddComponent<AttackDamageComponent>(m_pCurrentWeapon->GetDamage(), m_pColliderManager);
            
            melee.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(playerScale));
            melee.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition() + offset);
            auto& meleeTD = melee.AddComponent<TimedDestroyerComponent>(1,1);

            auto& meleeVelocity = melee.AddComponent<VelocityComponent>();
            meleeVelocity.SetVelocity(glm::vec2(0.0f, 0.0f));
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

    if (m_isHoldingObject && wolf::Input::IsLMBHeld()) {
        RenderThrowPowerBar();
    }

    // Pop ImGui style variables and colors
    ImGui::PopStyleVar(3); // Pop style variables (WindowRounding, FrameRounding, and FramePadding)
    ImGui::PopStyleColor(3); // Pop style colors (WindowBg, Border, and BorderShadow)
}


// !-- Aurora added this --!
void PlayerController::HandleWeaponEquippedEvent(const WeaponEquippedEvent& p_event) {
    printf("The player equipped a %s!\n", p_event.pWeapon->GetName().c_str());
    this->m_pCurrentWeapon = p_event.pWeapon;
}

void PlayerController::HandleWeaponUnequippedEvent(const WeaponUnequippedEvent& p_event)
{
    if(this->m_pCurrentWeapon == p_event.pWeapon)
    {
        this->m_pCurrentWeapon = this->m_pDefaultWeapon;
    }
}

void PlayerController::HandleArmourEquippedEvent(const ArmourEquippedEvent& p_event) {
    printf("The player equipped a %s!\n", p_event.pArmour->GetName().c_str());
}

void PlayerController::RenderThrowPowerBar() {
    // Position the power bar on the right side of the screen
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 basePos = ImVec2(displaySize.x - 120.0f, displaySize.y / 2.0f - 50.0f); // Right side, centered vertically

    // Push ImGui styles for a more vibrant look with background, rounded frame, and padding
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);           // Rounded corners for the frame
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);          // Rounded corners for the window
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 2.0f)); // Padding inside the bar for a thicker look
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // Semi-transparent black background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));   // Soft white border

    // Render background bar with a slightly larger size for a frame effect
    ImGui::SetNextWindowPos(ImVec2(basePos.x - 5.0f, basePos.y - 5.0f));
    ImGui::SetNextWindowSize(ImVec2(110.0f, 18.0f));
    ImGui::Begin("##PowerBarBackground", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
    ImGui::End();

    // Render the throw power bar
    ImGui::SetNextWindowPos(basePos);
    ImGui::SetNextWindowSize(ImVec2(100.0f, 15.0f));
    ImGui::Begin("##PowerBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImVec4 barColor = ImVec4(1.0f - (m_throwPower / m_maxThrowPower), (m_throwPower / m_maxThrowPower), 0.0f, 1.0f); // Gradient from red to green
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(m_throwPower / m_maxThrowPower, ImVec2(-1, 10.0f));
    ImGui::PopStyleColor();
    ImGui::End();

    // Render label "Power" below the bar
    ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y - 20.0f));
    ImGui::SetNextWindowSize(ImVec2(100.0f, 10.0f));
    ImGui::Begin("##PowerLabel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Power");
    ImGui::End();

    // Pop all the style vars and colors
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}