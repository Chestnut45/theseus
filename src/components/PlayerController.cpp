
//-----------------------------------------------------------------------------
// File: PlayerController.cpp
// Original Author: Youssef Ashraf
// Modifications: Nguyễn Minh Nhật, D'Anyil Landry, Aurora Ryder
// ver 2.0. Updated to remove deprecated hitbox and hurtbox components.
//-----------------------------------------------------------------------------

#include "AttackDamageComponent.h"
#include "ColliderComponent.h"
#include "HealthComponent.h"
#include "VelocityComponent.h"
#include "StatusComponent.h"
#include "TimedDestroyerComponent.h"
#include "HarpyController.h"
#include "MinitaurController.h"
#include "GorgonController.h"
#include "PlayerController.h"
#include "LabyrinthManager.h"
#include "BossController.h"

#include "../DDACalculator.h"

#include "../GLShapesRenderer.h"
#include "../inventory/ItemCreator.h"

#include <W_Input.h>
#include <W_Logging.h>
#include <W_EventManager.h>
#include <W_Audio.h>

PlayerController::PlayerController() = default;

PlayerController::~PlayerController() {
    wolf::EventManager::RemoveListener<WeaponEquippedEvent, PlayerController, &PlayerController::HandleWeaponEquippedEvent>(*this);
    wolf::EventManager::RemoveListener<ArmourEquippedEvent, PlayerController, &PlayerController::HandleArmourEquippedEvent>(*this);
    wolf::EventManager::RemoveListener<WeaponUnequippedEvent, PlayerController, &PlayerController::HandleWeaponUnequippedEvent>(*this);
    wolf::EventManager::RemoveListener<ArmourUnequippedEvent, PlayerController, &PlayerController::HandleArmourUnequippedEvent>(*this);
    wolf::EventManager::RemoveListener<DamageEvent, PlayerController, &PlayerController::OnDamageEvent>(*this);
    if (m_deathScreenTexture) {
        wolf::TextureManager::DestroyTexture(m_deathScreenTexture);
        m_deathScreenTexture = nullptr;
    }
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

PlayerController::PlayerAction PlayerController::GetPlayerAction() const
{
    return this->m_action;
}

void PlayerController::SetAction(PlayerAction action)
{
    // If transitioning to THROWING or PICKING_UP state, reset any active attack
    if ((action == PlayerAction::THROWING || action == PlayerAction::PICKING_UP) && m_action == PlayerAction::ATTACKING)
    {
        m_action = PlayerAction::NONE; // Reset to NONE to avoid conflict
    }

    // End old action
    switch (m_action)
    {
        case PlayerAction::ATTACKING:
        {
            EndAttacking();
            break;
        }
        case PlayerAction::PETRIFIED:
        {
            EndPetrified();
            break;
        }
        case PlayerAction::ROLLING:
        {
            EndRoll();
            break;
        }
        default:
        {
            break;
        }
    }

    // Start new action
    switch (action)
    {
        case PlayerAction::ATTACKING:
        {
            StartAttack();
            break;
        }
        case PlayerAction::PETRIFIED:
        {
            StartPetrified();
            break;
        }
        case PlayerAction::ROLLING:
        {
            StartRoll();
            break;
        }
        case PlayerAction::DEAD:
        {
            StartDeath();
            break;
        }
        default:
        {
            break;
        }
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
    m_runtimeTimer.Start(); // Start runtime timer

    // Grab collider component
    m_pCollider = pGameObject->GetComponent<ColliderComponent>();

    m_attackTimer.Start();

    InitializeAnimations();

    // !-- Aurora added this --!
    wolf::EventManager::AddListener<WeaponEquippedEvent, PlayerController, &PlayerController::HandleWeaponEquippedEvent>(*this);
    wolf::EventManager::AddListener<WeaponUnequippedEvent, PlayerController, &PlayerController::HandleWeaponUnequippedEvent>(*this);
    wolf::EventManager::AddListener<ArmourEquippedEvent, PlayerController, &PlayerController::HandleArmourEquippedEvent>(*this);
    wolf::EventManager::AddListener<ArmourUnequippedEvent, PlayerController, &PlayerController::HandleArmourUnequippedEvent>(*this);

    // Listen for damage events
    wolf::EventManager::AddListener<DamageEvent, PlayerController, &PlayerController::OnDamageEvent>(*this);
}

glm::vec2 PlayerController::GetLastFacingDirectionVector() const 
{
    switch (m_lastFaceDirectionEnum) {
        case PlayerDirection::NORTH:       return glm::vec2(0.0f, 1.0f);
        case PlayerDirection::EAST:        return glm::vec2(1.0f, 0.0f);
        case PlayerDirection::SOUTH:       return glm::vec2(0.0f, -1.0f);
        case PlayerDirection::WEST:        return glm::vec2(-1.0f, 0.0f);
        case PlayerDirection::NORTH_EAST:  return glm::normalize(glm::vec2(1.0f, 1.0f));
        case PlayerDirection::NORTH_WEST:  return glm::normalize(glm::vec2(-1.0f, 1.0f));
        case PlayerDirection::SOUTH_EAST:  return glm::normalize(glm::vec2(1.0f, -1.0f));
        case PlayerDirection::SOUTH_WEST:  return glm::normalize(glm::vec2(-1.0f, -1.0f));
        default:                           return glm::vec2(1.0f, 0.0f);
    }
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
    m_pAnimComponent->SetLayer(10);
}

// Main update loop for the player controller
void PlayerController::Update(float delta)
{
    if (!m_active) return;

    auto* pGameObject = GetGameObject();
    if (!pGameObject) return;

    // Retrieve the active camera through the game's scene using the game object
    auto* pCamera = pGameObject->GetScene().GetActiveCamera();
    if (pCamera && m_debugHotkeys)
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

    // Update invulnerability window
    if (!m_pCollider->IsHurtbox() && m_invulnTimer.Elapsed() > m_invulnSeconds)
    {
        m_pCollider->SetColliderType(ColliderComponent::ColliderType::HITHURTBOXDR);
        m_invulnTimer.Reset();
    }

    auto* pInventory = pGameObject->GetComponent<PlayerInventoryComponent>();
    if (pInventory)
    {
        m_inventoryOpen = pInventory->IsOpen();
        m_inventoryHovered = pInventory->IsToggleButtonHovered();
    }

    if (m_action != PlayerAction::DEAD)
    {
        RegenerateStamina(delta);
        // Call SetAnimationBasedOnState() only if the action or direction has changed
        if (m_action != m_previousAction || m_lastMoveDirectionEnum != m_previousDirection)
        {
            SetAnimationBasedOnState();
            m_previousAction = m_action;
            m_previousDirection = m_lastMoveDirectionEnum;
        }

        HandlePlayerInput(delta);

        // If holding an object, handle throw/drop actions
        if (m_isHoldingObject) {
            HandleThrowing(delta);  // Throw if needed
            HandleMovement(delta);  // Continue to allow movement
            return;  // Skip attack or other actions while holding an object
        }

        // Check if player is petrified
        StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
        if
        (
            statusComponent != nullptr                                                          &&
            statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)
        )
        {
            if(m_action != PlayerAction::PETRIFIED)
            {
                SetAction(PlayerAction::PETRIFIED);
            }
        }
        else
        {
            // On exiting petrified state
            if(m_action == PlayerAction::PETRIFIED)
            {
                SetAction(PlayerAction::NONE);
            } 
        }
        CheckHealth();
    }

    // Handle regular player actions
    switch (m_action) {
        case PlayerAction::ATTACKING:
            HandleAttacking(delta);
            HandleMovement(delta);
            break;
        case PlayerAction::PICKING_UP:
            SetAction(PlayerAction::NONE);  // Transition to NONE after picking up
            break;
        case PlayerAction::ROLLING:
            HandleRolling(delta);
            break;
        case PlayerAction::THROWING:
            HandleThrowing(delta);  // Handle throw logic
            HandleMovement(delta);
            break;
        case PlayerAction::PETRIFIED:
            HandlePetrified(delta);
            break;
        case PlayerAction::DEAD:
            HandleDeath(delta);
            break;
        default:
            HandleJumping(delta);
            HandleMovement(delta);
            break;
    }
}

void PlayerController::HandlePlayerInput(float delta)
{
    if (m_debugHotkeys)
    {
        // Godmode hotkey
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_PAGE_DOWN))
        {
            m_godmode = !m_godmode;
        }

        // Status effect hotkeys
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_UP))
        {
            StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
            statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::HEALING, 5.0f);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_DOWN))
        {
            StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
            statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 5.0f);
        }
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_LEFT))
        {
            StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
            statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::BURNING, 5.0f);
        }

        if (wolf::Input::IsKeyJustDown(GLFW_KEY_RIGHT))
        {
            StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
            statusComponent->AddStatusEffect(StatusComponent::StatusEffectType::POISONED, 5.0f);
        }

        // Super speed hotkey
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_PAGE_UP))
        {
            m_superSpeed = !m_superSpeed;
            if (m_superSpeed)
            {
                m_normalMoveSpeed = 800.0f;
                m_rollSpeed = 1600.0f;
                m_inventoryMoveSpeed = 400.0f;
            }
            else
            {
                m_normalMoveSpeed = 200.0f;
                m_rollSpeed = 400.0f;
                m_inventoryMoveSpeed = 100.0f;
            }

            // Update current speed
            m_currentMoveSpeed = m_action == PlayerAction::IN_INVENTORY ? m_inventoryMoveSpeed : m_normalMoveSpeed;
        }

        // Teleport to labyrinth spawn location hotkey
        if (wolf::Input::IsKeyJustDown(GLFW_KEY_HOME))
        {
            // Teleport to the spawn position
            for (const auto&&[_, labMan] : GetGameObject()->GetScene().Each<LabyrinthManager>())
            {
                GetGameObject()->GetComponent<wolf::Transform2D>()->SetPosition(labMan.GetSpawnLocation());
                break;
            }
        }
    }

    auto* playerInventory = GetGameObject()->GetComponent<PlayerInventoryComponent>();

    // Handle inventory management with left alt
    if (wolf::Input::IsKeyDown(GLFW_KEY_LEFT_ALT)) {
        if (m_action != PlayerAction::IN_INVENTORY) {
            playerInventory->Open();
            m_currentMoveSpeed = m_inventoryMoveSpeed;
        }
    } else if (wolf::Input::IsKeyReleased(GLFW_KEY_LEFT_ALT)) {
        playerInventory->Close();
        m_currentMoveSpeed = m_normalMoveSpeed;
    }
    glm::vec2 direction = GetLastFacingDirectionVector();

    // Only start roll if the following conditions are met
    bool attackingCondition = m_action != PlayerAction::ATTACKING || m_pCurrentWeapon->GetWeaponType() == WeaponType::BOW;
    if (
        wolf::Input::IsKeyJustDown(GLFW_KEY_SPACE)  && 
        m_action != PlayerAction::ROLLING           && 
        attackingCondition                          && 
        m_action != PlayerAction::PETRIFIED         && 
        m_stamina >= 15.0f && !m_isHoldingObject
    )
    {
        m_pAnimComponent->SetAnimPaused(false);
        SetAction(PlayerAction::ROLLING);
        return;
    }

    // Start the attack if the following conditions are met
    // !-- Aurora added a m_pCurrentWeapon != nullptr check here --!
    if (
        wolf::Input::IsLMBJustDown()                            && 
        m_pCurrentWeapon                                        && 
        m_action != PlayerAction::ATTACKING                     &&
        m_action != PlayerAction::PETRIFIED                     &&
        m_attackTimer.Elapsed() >= m_pCurrentWeapon->GetDelay() && 
        !m_inventoryOpen                                        && 
        !m_inventoryHovered)
    {
        SetAction(PlayerAction::ATTACKING);
    }
    
    // Handle pick up and drop actions
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_E)) {
        PickUpObject();
    }
    if (wolf::Input::IsKeyJustDown(GLFW_KEY_Q)) {
        DropObject();
    }
}

void PlayerController::PickUpObject() {
    // If the player is rolling, reset the rolling state before picking up an object
    if (m_action == PlayerAction::ROLLING) {
        EndRoll(); // Ensure rolling-related mechanics are stopped
        SetAction(PlayerAction::NONE);
    }

    // Attempt to pick up a nearby throwable object
    for (auto&& [entity, throwable] : GetGameObject()->GetScene().Each<ThrowableObjectComponent>()) {
        if (throwable.IsCloseToPlayer(150.0f)) {  // Check proximity
            throwable.PickUp();
            m_pHeldObject = &throwable;           // Store reference to the held object
            m_isHoldingObject = true;
            SetAction(PlayerAction::PICKING_UP);  // Temporary state while picking up
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

void PlayerController::HandlePetrified(float delta)
{
        
}

void PlayerController::HandleDeath(float delta)
{
    // Fall over
    if(m_fallDeadTimer <= m_timeToFallDead)
    {
        if(m_fallDeadTimer == 0.0f)
        {
            if (m_pVelocity)
            {
                m_pVelocity->SetVelocity(glm::vec2(0.0f));
            }

            ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
            if(collider != nullptr)
            {
                collider->SetColliderType(ColliderComponent::ColliderType::NONE);
            }
            
            m_pAnimComponent->SetTint(glm::vec3(1,0,0));
        }

        float angle = (90.0f / m_timeToFallDead) * delta;
        m_pTransform->RotateDegrees(angle);
        
        m_fallDeadTimer += delta;
    }

    // Lie dead
    else
    {
        if(m_lieDeadTimer >= m_timeToLieDead)
        {
        }
        m_lieDeadTimer += delta;
    }
}


void PlayerController::HandleBowAttack(float delta)
{
    // Play sfx if just clicked
    if (wolf::Input::IsLMBJustDown()) wolf::Audio::Play("data/sounds/sfx_bow_loading.wav", 0.9f);

    // Charging bow
    if(wolf::Input::IsLMBDown())
    {
        if(m_bIsChargingOver == false)
        {
            // Calculate scale of charge power
            m_bowChargeScale += delta * m_bowChargeRate;
            m_bowChargeScale = std::min(m_bowChargeScale, m_bowMaxChargeScale);

            // Calculate range of arrow
            m_arrowRange += delta * m_bowChargeRate * m_arrowMaxRange;
            m_arrowRange = std::min(m_arrowRange, m_arrowMaxRange);

            CalculateAttackDirection();
            
            // Update face direction
            glm::vec2 lastDir = ClampDirection(m_attackDir);
            m_lastFaceDirectionEnum = GetDirectionFromVector(lastDir);
            
            // Calculate range indicator
            HandleBowRangeIndicator(delta);
        }
    }
    else
    {
        m_bIsChargingOver = true;
    }

    // Firing arrow
    if(m_bIsChargingOver) 
    {        
        if(m_currentBowAnim == 1 && m_pAnimComponent->IsAnimationFinished() == true)
        {    
            // Attack
            auto* player = this->GetGameObject();
            if (!player || !m_pTransform) return;

            glm::vec2 playerScale = player->GetComponent<wolf::Transform2D>()->GetGlobalScale();
            VelocityComponent* playerVelocityComponent = player->GetComponent<VelocityComponent>();
            glm::vec2 playerVelocity = playerVelocityComponent == nullptr ? glm::vec2(0.0f) : playerVelocityComponent->GetVelocity();
            glm::vec2 playerDirection = GetVectorFromDirection(this->m_lastFaceDirectionEnum);
            glm::vec2 spawnOffset;

            // Set data for projectile collider
            ProjectileProperties projprop = m_pCurrentWeapon->GetProjectileProperties();

            glm::vec2 projectileDimensions = projprop.v2HurtboxSize;
            glm::vec2 hurtboxOffset = glm::vec2(-projectileDimensions.x, projectileDimensions.y) * 0.5f;

            // Spawn projectile object & add components
            auto& scene = player->GetScene();
            auto& projectile = scene.CreateObject2D();

            // Add sprite component
            auto& projectileSprite = projectile.AddComponent<wolf::Sprite2D>(projprop.strPathToSprite);
            projectileSprite.SetOriginToCenterOfTexture();
            
            // Add collider component
            auto& projectileCollider = projectile.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 1);
            projectileCollider.AddColliderBox(projectileDimensions, hurtboxOffset);
            projectileCollider.SetIgnoreTag(player->GetID());
            
            // Add attack damage component
            float damage = glm::max(m_pCurrentWeapon->GetDamage() * 0.01f, m_pCurrentWeapon->GetDamage() * m_bowChargeScale);
            auto& projectileADComponent = projectile.AddComponent<AttackDamageComponent>(damage, m_pColliderManager, 200);
            
            // Calculate spawn offset
            spawnOffset.x = spawnOffset.x > 0.0f ? (spawnOffset.x + projectileDimensions.x * 0.5f) : ( spawnOffset.x < 0.0f ? (spawnOffset.x - projectileDimensions.x * 0.5f) : (spawnOffset.x));
            spawnOffset.y = spawnOffset.y > 0.0f ? (spawnOffset.y + projectileDimensions.y * 0.5f) : ( spawnOffset.y < 0.0f ? (spawnOffset.y - projectileDimensions.y * 0.5f) : (spawnOffset.y));
            projectile.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
            
            // Calculate projectile velocity
            float arrowSpeed = glm::max(glm::length(projprop.v2Velocity) * 0.4f, glm::length(projprop.v2Velocity) * m_bowChargeScale);
            auto& projectileVelocity = projectile.AddComponent<VelocityComponent>();
            projectileVelocity.SetVelocity(m_attackDir * arrowSpeed);

            // Add timed destroyer component
            float time = m_arrowRange / arrowSpeed + 0.5f;
            auto& projectileTDComponent = projectile.AddComponent<TimedDestroyerComponent>(time);

            // Calculate how to rotate arrow sprite
            glm::vec2 baseVector = glm::vec2(1.0f, 0.0f);
            float angle = std::acos(glm::dot(baseVector, m_attackDir) / (glm::length(baseVector) * glm::length(m_attackDir)));
            if(m_attackDir.y < 0.0f) angle *= -1;
            projectile.GetComponent<wolf::Transform2D>()->SetRotation(angle);

            projectile.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(3.0f));

            // Play sfx
            wolf::Audio::Play("data/sounds/sfx_arrow_shot.wav", 0.5f);
        }
    }
    HandleBowAttackAnimation();
}

void PlayerController::HandleSpearAttack(float delta)
{
    if(m_attackTimer.Elapsed() >= m_pCurrentWeapon->GetDelay())
    {
        // Attack
        auto* player = this->GetGameObject();
        if (!player || !m_pTransform) return;

        glm::vec2 playerScale = player->GetComponent<wolf::Transform2D>()->GetGlobalScale();
        VelocityComponent* playerVelocityComponent = player->GetComponent<VelocityComponent>();
        glm::vec2 playerVelocity = playerVelocityComponent == nullptr ? glm::vec2(0.0f) : playerVelocityComponent->GetVelocity();
        glm::vec2 playerDirection = GetVectorFromDirection(this->m_lastFaceDirectionEnum);
        glm::vec2 spawnOffset;

        // Set & calculate data for melee collider
        glm::vec2 meleeDimensions = m_pCurrentWeapon->GetHurtBoxSize();
        glm::vec2 offset = glm::vec2(0.0f, 0.0f);
        
        // Horizontal attack
        if(playerDirection.x != 0.0f && playerDirection.y == 0.0f)
        {
            meleeDimensions.x *= 1.25f;
            offset.x = playerDirection.x > 0.0f ? 0.0f : -meleeDimensions.x;
            offset.y = meleeDimensions.y * 0.5f;
        }

        // Vertical attack
        else if(playerDirection.x == 0.0f && playerDirection.y != 0.0f)
        {
            meleeDimensions.y *= 1.25f;
            offset.x = -meleeDimensions.x * 0.5f;
            offset.y = playerDirection.y > 0.0f ? meleeDimensions.y : 0.0f;
        }

        // Diagonal attack
        else if(playerDirection.x != 0.0f && playerDirection.y != 0.0f)
        {
            offset.x = -meleeDimensions.x * 0.5f;
            offset.x = playerDirection.x > 0.0f ? 0.0f : -meleeDimensions.x;
            offset.y = meleeDimensions.y * 0.5f;
            offset.y = playerDirection.y > 0.0f ? meleeDimensions.y : 0.0f;
        }

        // Scale offset by player scale
        offset *= playerScale;

        auto& scene = player->GetScene();

        // Create melee object & add components
        auto& melee = scene.CreateObject2D();

        auto& meleeCollider = melee.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 0);
        meleeCollider.AddColliderBox(meleeDimensions * playerScale, offset);
        meleeCollider.SetIgnoreTag(player->GetID());

        auto& meleeADcomponent = melee.AddComponent<AttackDamageComponent>(m_pCurrentWeapon->GetDamage(), m_pColliderManager, 2000.0f);
        
        melee.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(playerScale));
        melee.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
        auto& meleeTD = melee.AddComponent<TimedDestroyerComponent>(1,1);

        m_attackTimer.Restart();

        // Play pitched down sword whoosh
        wolf::Audio::Play("data/sounds/sfx_sword_whoosh.wav", 0.7f, -10000.0f);
    }
    HandleAttackAnimation();
}

void PlayerController::HandleSwordAttack(float delta)
{
    if(m_attackTimer.Elapsed() >= m_pCurrentWeapon->GetDelay())
    {
        // Attack
        auto* player = this->GetGameObject();
        if (!player || !m_pTransform) return;

        glm::vec2 playerScale = player->GetComponent<wolf::Transform2D>()->GetGlobalScale();
        VelocityComponent* playerVelocityComponent = player->GetComponent<VelocityComponent>();
        glm::vec2 playerVelocity = playerVelocityComponent == nullptr ? glm::vec2(0.0f) : playerVelocityComponent->GetVelocity();
        glm::vec2 playerDirection = GetVectorFromDirection(this->m_lastFaceDirectionEnum);
        glm::vec2 spawnOffset;

        // Set & calculate data for melee collider
        glm::vec2 meleeDimensions = m_pCurrentWeapon->GetHurtBoxSize();
        glm::vec2 offset = glm::vec2(0.0f, 0.0f);
        
        // Horizontal attack
        if(playerDirection.x != 0.0f && playerDirection.y == 0.0f)
        {  
            meleeDimensions.y *= 1.25f;
            offset.x = playerDirection.x > 0.0f ? 0.0f : -meleeDimensions.x;
            offset.y = meleeDimensions.y * 0.5f;
        }

        // Vertical attack
        else if(playerDirection.x == 0.0f && playerDirection.y != 0.0f)
        {
            meleeDimensions.x *= 1.25f;
            offset.x = -meleeDimensions.x * 0.5f;
            offset.y = playerDirection.y > 0.0f ? meleeDimensions.y : 0.0f;
        }
        
        // Diagonal attack
        else if(playerDirection.x != 0.0f && playerDirection.y != 0.0f)
        {
            offset.x = -meleeDimensions.x * 0.5f;
            offset.x = playerDirection.x > 0.0f ? 0.0f : -meleeDimensions.x;
            offset.y = meleeDimensions.y * 0.5f;
            offset.y = playerDirection.y > 0.0f ? meleeDimensions.y : 0.0f;
        }

        // Scale offset by player scale
        offset *= playerScale;

        auto& scene = player->GetScene();

        // Create melee object & add components
        auto& melee = scene.CreateObject2D();

        auto& meleeCollider = melee.AddComponent<ColliderComponent>(ColliderComponent::ColliderType::HURTBOXDD, 1, 0);
        meleeCollider.AddColliderBox(meleeDimensions * playerScale, offset);
        meleeCollider.SetIgnoreTag(player->GetID());

        auto& meleeADcomponent = melee.AddComponent<AttackDamageComponent>(m_pCurrentWeapon->GetDamage(), m_pColliderManager, 2000.0f);
        
        melee.GetComponent<wolf::Transform2D>()->SetScale(glm::vec2(playerScale));
        melee.GetComponent<wolf::Transform2D>()->SetPosition(player->GetComponent<wolf::Transform2D>()->GetGlobalPosition());
        auto& meleeTD = melee.AddComponent<TimedDestroyerComponent>(1,1);    

        m_attackTimer.Restart();

        // Play sound effect
        wolf::Audio::Play("data/sounds/sfx_sword_whoosh.wav", 0.7f);
    }
    HandleAttackAnimation();
}

void PlayerController::HandleAttackAnimation()
{
    // If the animation has finished, transition out of the attacking state
    if (m_pAnimComponent->IsAnimationFinished())
    {
        // Determine the next action based on the player's velocity
        if (glm::length(m_pVelocity->GetVelocity()) < 0.01f)
        {
            SetAction(PlayerAction::NONE); // Set to idle state
        }
        else
        {
            SetAction(PlayerAction::WALKING); // Set to walking state
        }

        // Clear the current animation and set a new one based on the updated state
        // This goes here rather than EndAttack() as SetAnimationBasedOnState() needs to happen before SetAction()
        m_currentAnimation = "";
        SetAnimationBasedOnState();
    }
}

void PlayerController::HandleBowAttackAnimation()
{
    // If charging over
    if(m_bIsChargingOver)
    {
        // If current anim is 'loading bow'
        if(m_currentBowAnim == 0)
        {
            // If current anim finished, switch to 'firing bow'
            if(m_pAnimComponent->IsAnimationFinished() == true)
            {
                m_currentBowAnim = 1;

                std::string sheet = "BowFire";
                
                switch (m_lastFaceDirectionEnum)
                {
                    case PlayerDirection::SOUTH:       {sheet += "South"; break;}
                    case PlayerDirection::EAST:        {sheet += "East"; break;}
                    case PlayerDirection::NORTH:       {sheet += "North"; break;}
                    case PlayerDirection::WEST:        {sheet += "West"; break;}
                    case PlayerDirection::NORTH_EAST:  {sheet += "East"; break;}
                    case PlayerDirection::NORTH_WEST:  {sheet += "West"; break;}
                    case PlayerDirection::SOUTH_EAST:  {sheet += "East"; break;}
                    case PlayerDirection::SOUTH_WEST:  {sheet += "West"; break;}
                    default:                           {sheet += "South"; break;}
                }

                m_pAnimComponent->SetAnimation(sheet);
                m_pAnimComponent->SetAnimPaused(false);
            }
        }
        // If current anim is 'firing bow'
        else
        {
            m_pAnimComponent->SetAnimPaused(false);

            // If current anim finished
            if(m_pAnimComponent->IsAnimationFinished() == true)
            {                
                // Determine the next action based on the player's velocity
                if (glm::length(m_pVelocity->GetVelocity()) < 0.01f)
                {
                    SetAction(PlayerAction::NONE); // Set to idle state
                }
                else
                {
                    SetAction(PlayerAction::WALKING); // Set to walking state
                }

                // Clear the current animation and set a new one based on the updated state
                // This goes here rather than EndAttack() as SetAnimationBasedOnState() needs to happen before SetAction()
                m_currentAnimation = "";
                SetAnimationBasedOnState();
            }
        }
    }
    // if still charging
    else
    {
        // If current anim is 'loading bow'
        if(m_currentBowAnim == 0)
        {
            // If current anim finished, switch to 'firing bow' & pause
            if(m_pAnimComponent->IsAnimationFinished() == true)
            {
                m_currentBowAnim = 1;
                m_pAnimComponent->SetAnimPaused(true);
            }
        }
        // If current anim is 'firing bow', update sprite to face direction of cursor
        else
        {
            std::string sheet = "BowFire";
                
            switch (m_lastFaceDirectionEnum)
            {
                case PlayerDirection::SOUTH:       { sheet += "South"; break; }
                case PlayerDirection::EAST:        { sheet += "East"; break;  }
                case PlayerDirection::NORTH:       { sheet += "North"; break; }
                case PlayerDirection::WEST:        { sheet += "West"; break;  }
                case PlayerDirection::NORTH_EAST:  { sheet += "East"; break;  }
                case PlayerDirection::NORTH_WEST:  { sheet += "West"; break;  }
                case PlayerDirection::SOUTH_EAST:  { sheet += "East"; break;  }
                case PlayerDirection::SOUTH_WEST:  { sheet += "West"; break;  }
                default:                           { sheet += "South"; break; }
            }            
            m_pAnimComponent->SetAnimation(sheet);
        }
    }
}

void PlayerController::HandleBowRangeIndicator(float delta)
{
    glm::vec2 playerPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endpoint = playerPos + m_attackDir * m_arrowRange;   // m_attackDir is already normalised
    glm::vec2 trueEndpoint = DDACalculator::GetInstance()->GetEndpoint(playerPos, endpoint);

    glm::vec4 colour = m_bowRangeIndicatorColour;
    // std::cout << colour.r << ", " << colour.g << ", " << colour.b << ", " << colour.a << std::endl;

    GLShapesRenderer::GetInstance()->AddLine(
                                            {playerPos.x, playerPos.y, colour.r, colour.g, colour.b, colour.a}, 
                                            {trueEndpoint.x, trueEndpoint.y, colour.r, colour.g, colour.b, colour.a}
                                            );

}

void PlayerController::CalculateAttackDirection()
{
    wolf::Scene* scene = &this->GetGameObject()->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    glm::vec2 cameraPos = camera->GetPosition();
    glm::vec2 viewSize = camera->GetViewSize();
    glm::vec2 worldPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
    glm::vec2 cursorScreenPos = wolf::Input::GetMousePos();
    glm::vec2 cursorWorldPos = glm::vec2
    (
        cameraPos.x + (cursorScreenPos.x - viewSize.x * 0.5f),
        cameraPos.y + (viewSize.y * 0.5f - cursorScreenPos.y)
    );

    m_attackDir = glm::normalize(cursorWorldPos - worldPos);
}

// Code taken from RenderThrowPowerBar() by Youssef
void PlayerController::RenderBowPowerBar()
{
    // Get positions
    wolf::Scene* scene = &this->GetGameObject()->GetScene();
    wolf::Camera2D* camera = scene->GetActiveCamera();
    glm::vec2 cameraPos = camera->GetPosition();
    glm::vec2 viewSize = camera->GetViewSize();
    glm::vec2 viewSizeHalf = glm::vec2(viewSize.x * 0.5f, viewSize.y * 0.5f);
    glm::vec2 playerPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    // Calculate position of power bar in world space
    glm::vec2 barPos = playerPos - glm::vec2(BOW_POWER_BAR_SIZE.x, BOW_POWER_BAR_SIZE.y) * 0.5f;
    barPos.y += 32.0f * LabyrinthManager::SCALE;

    glm::vec2 screenPos;
    screenPos.x = (barPos.x - (cameraPos.x - viewSizeHalf.x));
    screenPos.y = (barPos.y - (cameraPos.y - viewSizeHalf.y)) * (-1) + viewSize.y;

    // Push ImGui styles for a more vibrant look with background, rounded frame, and padding
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);           // Rounded corners for the frame
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);          // Rounded corners for the window
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 2.0f)); // Padding inside the bar for a thicker look
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // Semi-transparent black background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));   // Soft white border

    // Render background bar with a slightly larger size for a frame effect
    ImGui::SetNextWindowPos(ImVec2(screenPos.x - 5.0f, screenPos.y - 5.0f));
    ImGui::SetNextWindowSize(ImVec2(THROW_POWER_BAR_SIZE.x + 10.0f, THROW_POWER_BAR_SIZE.y + 3.0f));
    ImGui::Begin("##PowerBarBackground", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
    ImGui::End();

    // Render the bow power bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
    ImGui::SetNextWindowSize(BOW_POWER_BAR_SIZE);
    ImGui::Begin("##PowerBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImVec4 barColor = ImVec4(1.0f - (m_bowChargeScale / m_bowMaxChargeScale), (m_bowChargeScale / m_bowMaxChargeScale), 0.0f, 1.0f); // Gradient from red to green
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(m_bowChargeScale / m_bowMaxChargeScale, ImVec2(-1, 10.0f));
    ImGui::PopStyleColor();
    ImGui::End();

    // Render label "Power" below the bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y - 20.0f));
    ImGui::SetNextWindowSize(ImVec2(100.0f, 10.0f));
    ImGui::Begin("##PowerLabel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Power");
    ImGui::End();

    // Pop all the style vars and colors
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

glm::vec2 PlayerController::ClampDirection(const glm::vec2& direction) const
{
    glm::vec2 finalDir = glm::vec2(0.0f, 0.0f);
    float angle = std::atan2(direction.y, direction.x);
    float eighthPi = std::numbers::pi / 8.0f;

    if (angle >= -eighthPi && angle < eighthPi) 
    {
        finalDir = glm::normalize(glm::vec2(1.0f, 0.0f));    // East
    }
    else if (angle >= eighthPi && angle < 3.0f * eighthPi) 
    {
        finalDir = glm::normalize(glm::vec2(1.0f, 1.0f));    // NorthEast
    }
    else if (angle >= 3.0f * eighthPi && angle < 5.0f * eighthPi) {
        finalDir = glm::normalize(glm::vec2(0.0f, 1.0f));    // North
    }
    else if (angle >= 5.0f * eighthPi && angle < 7.0f * eighthPi) {
        finalDir = glm::normalize(glm::vec2(-1.0f, 1.0f));   // NorthWest
    }
    else if (angle >= 7.0f * eighthPi || angle < -7.0f * eighthPi) {
        finalDir = glm::normalize(glm::vec2(-1.0f, 0.0f));   // West
    }
    else if (angle >= -7.0f * eighthPi && angle < -5.0f * eighthPi) {
        finalDir = glm::normalize(glm::vec2(-1.0f, -1.0f));  // SouthWest
    }
    else if (angle >= -5.0f * eighthPi && angle < -3.0f * eighthPi) {
        finalDir = glm::normalize(glm::vec2(0.0f, -1.0f));   // South
    }
    else if (angle >= -3.0f * eighthPi && angle < -eighthPi) {
        finalDir = glm::normalize(glm::vec2(1.0f, -1.0f));   // SouthEast
    }

    return finalDir;
}

void PlayerController::EndPetrified()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
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
    auto* throwableVelocity = m_pHeldObject->GetGameObject()->GetComponent<VelocityComponent>();
    if (!throwableVelocity) throwableVelocity = &m_pHeldObject->GetGameObject()->AddComponent<VelocityComponent>();
    
    glm::vec2 finalVelocity = throwDirection * m_throwPower * 3.0f + playerVelocity;
    throwableVelocity->SetVelocity(finalVelocity);

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
    
    // stop moving if the player is attacking with a bow
    if (m_action == PlayerAction::ATTACKING && m_pCurrentWeapon->GetWeaponType() == WeaponType::BOW)
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
        return;
    }
    glm::vec2 direction(0.0f);

    // Track and update currently held keys for smooth directional input
    direction.y += wolf::Input::IsKeyDown(GLFW_KEY_W) ? 1.0f : 0.0f;
    direction.y -= wolf::Input::IsKeyDown(GLFW_KEY_S) ? 1.0f : 0.0f;
    direction.x -= wolf::Input::IsKeyDown(GLFW_KEY_A) ? 1.0f : 0.0f;
    direction.x += wolf::Input::IsKeyDown(GLFW_KEY_D) ? 1.0f : 0.0f;
    
    // If player is not moving, attacking or rolling, then idle
    if (glm::length(direction) == 0.0f) {
        if (m_action != PlayerAction::ATTACKING && m_action != PlayerAction::ROLLING) SetAction(PlayerAction::NONE);
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        m_walkSoundTimer.Reset();
        return;
    }

    // Play walking sound effect
    static wolf::RNG rng;
    if (!m_walkSoundTimer.IsRunning()) m_walkSoundTimer.Start();
    if (m_walkSoundTimer.Elapsed() > m_walkSoundInterval) {
        wolf::Audio::Play("data/sounds/sfx_step.wav", 0.55f, rng.NextFloat(-10000.0f, -5000.0f));
        m_walkSoundTimer.Restart();
    }

    // Only update m_lastMoveDirectionEnum if our movement direction is not 0!
    direction = glm::length(direction) > 0.01f ? glm::normalize(direction) : glm::vec2(0.0f);
    m_lastMoveDirectionEnum = direction == glm::vec2(0.0f) ? m_lastMoveDirectionEnum : GetDirectionFromVector(direction);
    
    if(m_action == PlayerAction::IN_INVENTORY)
    {
        // printf("PlayerController - Inv_Open");
    }
    m_pVelocity->SetVelocity(direction * m_currentMoveSpeed);

    if (m_action != PlayerAction::ATTACKING && m_action != PlayerAction::ROLLING && !m_isJumping) SetAction(PlayerAction::WALKING);
}

// Manage attack state and animation transitions
void PlayerController::HandleAttacking(float delta)
{
    // Disable attacking when in inventory, picking up, holding
    // a throwable object, or when no weapon is equipped
    if (m_action == PlayerAction::IN_INVENTORY ||
        m_action == PlayerAction::PICKING_UP ||
        m_action == PlayerAction::THROWING ||
        !m_pCurrentWeapon) {
        return;
    }

    switch(this->m_pCurrentWeapon->GetWeaponType())
    {
        // Spawn projectile for bow
        case WeaponType::BOW:
        {
            HandleBowAttack(delta);
            break;
        }
        case WeaponType::SPEAR:
        {
            HandleSpearAttack(delta);
            break;
        }
        case WeaponType::SWORD:
        {
            HandleSwordAttack(delta);
            break;
        }
    }

}
// Handle rolling logic based on player input and stamina
void PlayerController::HandleRolling(float delta)
{
    // Prevent rolling if the player is in the inventory state
    if (m_action == PlayerAction::IN_INVENTORY) return;
    m_rollTimer -= delta;
    if (m_rollTimer <= 0.0f) SetAction(PlayerAction::NONE);
    return;
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
    if (m_action == PlayerAction::ATTACKING || m_action == PlayerAction::ROLLING || m_isJumping || m_action == PlayerAction::IN_INVENTORY) 
    {
        return;
    }
    

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

glm::vec2 PlayerController::GetVectorFromDirection(PlayerDirection direction) const
{
    glm::vec2 playerDirection = glm::vec2(0.0f, 0.0f);

    switch (direction)
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

    return playerDirection;
}

std::string PlayerController::GetAttackAnimationForDirection(PlayerDirection direction) const
{
    std::string weaponType = "Sword";
    std::string startingSheet = "Attack";
    WeaponType type = this->m_pCurrentWeapon->GetWeaponType();
    
    if(type == WeaponType::BOW) 
    {
        weaponType = "Bow";
        startingSheet = "Load";
    }
    
    else if(type == WeaponType::SPEAR)
    {
        weaponType = "Spear";
        startingSheet = "Attack";
    }
    else if(type == WeaponType::SWORD) 
    {
        weaponType = "Sword";
        startingSheet = "Attack";
    }

    switch (direction)
    {
        case PlayerDirection::SOUTH:       return weaponType + startingSheet + "South";
        case PlayerDirection::EAST:        return weaponType + startingSheet + "East";
        case PlayerDirection::NORTH:       return weaponType + startingSheet + "North";
        case PlayerDirection::WEST:        return weaponType + startingSheet + "West";
        case PlayerDirection::NORTH_EAST:  return weaponType + startingSheet + "East";
        case PlayerDirection::NORTH_WEST:  return weaponType + startingSheet + "West";
        case PlayerDirection::SOUTH_EAST:  return weaponType + startingSheet + "East";
        case PlayerDirection::SOUTH_WEST:  return weaponType + startingSheet + "West";
        default:                           return weaponType + startingSheet + "South";
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
    CalculateAttackDirection();
    glm::vec2 lastDir = ClampDirection(m_attackDir);
    m_lastFaceDirectionEnum = GetDirectionFromVector(lastDir);

    m_hasAppliedDamage = false;

    // Set the player action to attacking and reset attack-related timers.
    m_attackTimer.Restart();

    // Choose the correct animation based on the player's direction.
    std::string attackAnimation = GetAttackAnimationForDirection(m_lastFaceDirectionEnum);
    // Set the attacking animation.
    m_pAnimComponent->SetAnimation(attackAnimation);

    // Store the current animation to handle transitions later.
    m_currentAnimation = attackAnimation;

    // Reset bow attack-related members
    m_bowChargeScale = 0.0f;
    m_arrowRange = 0.0f;
    m_bIsChargingOver = false;
    m_currentBowAnim = 0;
}

void PlayerController::StartPetrified()
{
    // Values hardcoded based on 5s petrification attack from GorgonController, perhaps more sensible to centralise & handle effects in StatusComponent
    // TODO: Move SetSpecialEffects() calls involving petrification from PlayerController & all EnemyControllers to StatusComponent
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::MULTITEX_PETRIFIED, 0.1f, 4.8f, 0.1f);
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
}

void PlayerController::StartRoll()
{
    m_rollTimer = m_rollDuration;

    // ALWAYS Roll in the direction the player is inputting
    glm::vec2 rollDirection(0.0f);
    rollDirection.y += wolf::Input::IsKeyDown(GLFW_KEY_W) ? 1.0f : 0.0f;
    rollDirection.y -= wolf::Input::IsKeyDown(GLFW_KEY_S) ? 1.0f : 0.0f;
    rollDirection.x -= wolf::Input::IsKeyDown(GLFW_KEY_A) ? 1.0f : 0.0f;
    rollDirection.x += wolf::Input::IsKeyDown(GLFW_KEY_D) ? 1.0f : 0.0f;

    // Normalize
    rollDirection = glm::length(rollDirection) > 0.01f ? glm::normalize(rollDirection) : glm::vec2(0.0f);
    
    // Fallback to last facing dir
    if (rollDirection == glm::vec2(0.0f)) rollDirection = GetLastFacingDirectionVector();

    // Update velocity
    m_pVelocity->SetVelocity(rollDirection * m_rollSpeed);
    
    // Only take stamina if not in godmode
    if (!m_godmode)
    {
        m_stamina -= 25.0f;
        m_staminaRegenTimer.Restart();
    }

    // Compass direction animation names
    static const char* s_dirNames[] =
    {
        "East",
        "North",
        "West",
        "South"
    };

    // Determine compass direction text from roll direction
    float angle = -glm::atan(rollDirection.x, rollDirection.y);
    int dirIndex = (int)round(4 * angle / 6.28318530718f + 5) % 4;
    const char* const dirText = s_dirNames[dirIndex];
    std::string baseAnimName = "Roll";

    // Set roll animation
    m_pAnimComponent->SetAnimation(baseAnimName + dirText);

    // Update our cached animation name
    m_currentAnimation = baseAnimName + dirText;

    // Play sfx
    wolf::Audio::Play("data/sounds/sfx_roll.wav", 1.0f);
}

void PlayerController::EndRoll()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void PlayerController::StartJump()
{
    m_isJumping = true;
    m_jumpTimer = m_jumpHeight / m_jumpSpeed;
    SetAction(PlayerAction::JUMPING);
    m_pVelocity->SetVelocity(glm::vec2(0, m_jumpSpeed));
}

void PlayerController::EndAttacking()
{
    m_hasAppliedDamage = false;
}

void PlayerController::EndJump()
{
    m_isJumping = false;
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    SetAction(PlayerAction::NONE);
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

void PlayerController::Render(float delta)
{
    if (!m_active || !m_pTransform) return;
    if (m_action == PlayerAction::DEAD) {
        RenderDeathScreen();
        return;
    }

    float barWidth = 256.0f;
    float barHeight = 18.0f;
    float verticalOffset = 20.0f;  // Offset between health and stamina bars

    // Position both bars at the top-center of the screen
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImVec2 basePos = ImVec2(10.0f, 10.0f);

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
        float colorCoefficient = m_invulnTimer.IsRunning() ? 1.0f - m_invulnTimer.Elapsed() : 0.0f;

        // Update health color
        ImVec4 healthColor = ImVec4(1.0f, colorCoefficient,  colorCoefficient, 1.0f);

        // Render previous health fraction underneath to indicate damage taken
        m_prevHealthFraction += (healthComponent->GetHealth() / healthComponent->GetMaxHealth() - m_prevHealthFraction) * delta * 4.0f;

        ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y)); // Position for health bar
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##HealthBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor); // Deep red health color
        ImGui::ProgressBar(healthComponent->GetHealth() / healthComponent->GetMaxHealth(), ImVec2(-1, barHeight), "");
        ImGui::PopStyleColor(); // Pop color for health bar
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y)); // Position for health bar
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
        ImGui::Begin("##HealthBar2", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
        ImGui::ProgressBar(m_prevHealthFraction, ImVec2(-1.0f, barHeight));
        ImGui::PopStyleColor(); // Pop color for health bar
        ImGui::End();  
    }

    // Load the health bar frame image once
    static ImTextureID healthBarTextureID = nullptr;
    static wolf::Texture* pHealthBarTexture = nullptr;
    if (!pHealthBarTexture) {
        pHealthBarTexture = wolf::TextureManager::CreateTexture("data/textures/HealthBarFrame.png");
        healthBarTextureID = reinterpret_cast<void*>(pHealthBarTexture->GetID());
    }

    // Render the health bar frame on top of the actual bar
    ImGui::SetNextWindowPos({(basePos.x + barWidth) - pHealthBarTexture->GetWidth() - 9.0f, basePos.y - barHeight});
    ImGui::SetNextWindowSize({0,0});
    ImGui::Begin("HealthBarFrame", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
    ImGui::Image(healthBarTextureID, ImVec2(pHealthBarTexture->GetWidth(), pHealthBarTexture->GetHeight()), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();

    // Move the position down for the stamina bar
    basePos.y += (barHeight + verticalOffset);

    // Change the stamina bar height to be slightly smaller
    barHeight = 15.0f;

    // Render the stamina bar below the health bar
    ImGui::SetNextWindowPos(ImVec2(basePos.x, basePos.y)); // Position for stamina bar
    ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));
    ImGui::Begin("##StaminaBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green stamina color
    ImGui::ProgressBar(m_stamina / m_maxStamina, ImVec2(-1, barHeight)); // Full width, defined height
    ImGui::PopStyleColor(); // Pop color for stamina bar
    ImGui::End();

    // Load the stamina bar frame image once
    static ImTextureID staminaBarTextureID = nullptr;
    static wolf::Texture* pStaminaBarTexture = nullptr;
    if (!pStaminaBarTexture) {
        pStaminaBarTexture = wolf::TextureManager::CreateTexture("data/textures/StaminaBarFrame.png");
        staminaBarTextureID = reinterpret_cast<void*>(pStaminaBarTexture->GetID());
    }

    // Render the stamina bar frame on top of the actual bar
    ImGui::SetNextWindowPos({(basePos.x + barWidth) - pStaminaBarTexture->GetWidth() - 9.0f, basePos.y - barHeight - (barHeight / 2.0f)});
    ImGui::SetNextWindowSize({0,0});
    ImGui::Begin("StaminaBarFrame", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
    ImGui::Image(staminaBarTextureID, ImVec2(pStaminaBarTexture->GetWidth(), pStaminaBarTexture->GetHeight()), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::End();

    if (m_isHoldingObject && wolf::Input::IsLMBHeld()) {
        RenderThrowPowerBar();
    }

    if(m_action == PlayerAction::ATTACKING && m_pCurrentWeapon->GetWeaponType() == WeaponType::BOW)
    {
        RenderBowPowerBar();
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
        this->m_pCurrentWeapon = nullptr;
    }
}

void PlayerController::HandleArmourEquippedEvent(const ArmourEquippedEvent& p_event) {
    printf("The player equipped %s!\n", p_event.pArmour->GetName().c_str());

    //-----------------//
    //                 //
    //  Added by Nhat  //
    //                 //
    //-----------------//
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr)
    {
        for (auto info : *p_event.pArmour->GetStatusEffectList())
        {
            statusComponent->AddStatusEffect(info.enType, info.fDuration);
        }
        
        const float* info = p_event.pArmour->GetStatusEffectResistances();
        for (int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
        {
            StatusComponent::StatusEffectType seType = static_cast<StatusComponent::StatusEffectType>(i);
            statusComponent->SetStatusEffectResistance(seType, info[0]);
        }
    }

}

void PlayerController::HandleArmourUnequippedEvent(const ArmourUnequippedEvent& p_event)
{
    //-----------------//
    //                 //
    //  Added by Nhat  //
    //                 //
    //-----------------//
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr)
    {
        for (int i = 0; i < StatusComponent::StatusEffectType::NONE; i++)
        {
            StatusComponent::StatusEffectType seType = static_cast<StatusComponent::StatusEffectType>(i);
            statusComponent->SetStatusEffectResistance(seType, 0);
        }
    }
}

void PlayerController::OnDamageEvent(const DamageEvent& event)
{
    // React to damage and reset invulnerability timer
    if (event.m_pDamagedObject == GetGameObject())
    {
        m_invulnTimer.Restart();
        m_pCollider->SetColliderType(ColliderComponent::ColliderType::HITBOX);
        wolf::Audio::Play("data/sounds/sfx_oof.wav", 0.35f);
    }
    else
    {
        // TODO: Move out of here if we have time
        // Play hit sound effect when enemies are damaged
        if (event.m_pDamagedObject->HasAny<MinitaurController, GorgonController, HarpyController>())
        {
            wolf::Audio::Play("data/sounds/sfx_hit.wav", 0.15f);
        }

        if (event.m_pDamagedObject->HasAny<BossController>())
        {
            wolf::Audio::Play("data/sounds/sfx_hit_boss.wav", 0.8f);
        }
    }
}

void PlayerController::RenderThrowPowerBar() {
        // Get positions
        wolf::Scene* scene = &this->GetGameObject()->GetScene();
        wolf::Camera2D* camera = scene->GetActiveCamera();
        glm::vec2 cameraPos = camera->GetPosition();
        glm::vec2 viewSize = camera->GetViewSize();
        glm::vec2 viewSizeHalf = glm::vec2(viewSize.x * 0.5f, viewSize.y * 0.5f);
        glm::vec2 playerPos = this->GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
        // Calculate position of power bar in world space
        glm::vec2 barPos = playerPos - glm::vec2(BOW_POWER_BAR_SIZE.x, BOW_POWER_BAR_SIZE.y) * 0.5f;
        barPos.y += 32.0f * LabyrinthManager::SCALE;
    
        glm::vec2 screenPos;
        screenPos.x = (barPos.x - (cameraPos.x - viewSizeHalf.x));
        screenPos.y = (barPos.y - (cameraPos.y - viewSizeHalf.y)) * (-1) + viewSize.y;

    // Push ImGui styles for a more vibrant look with background, rounded frame, and padding
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);           // Rounded corners for the frame
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);          // Rounded corners for the window
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 2.0f)); // Padding inside the bar for a thicker look
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // Semi-transparent black background
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));   // Soft white border

    // Render background bar with a slightly larger size for a frame effect
    ImGui::SetNextWindowPos(ImVec2(screenPos.x - 5.0f, screenPos.y - 5.0f));
    ImGui::SetNextWindowSize(ImVec2(THROW_POWER_BAR_SIZE.x + 10.0f, THROW_POWER_BAR_SIZE.y + 3.0f));
    ImGui::Begin("##PowerBarBackground", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
    ImGui::End();

    // Render the throw power bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
    ImGui::SetNextWindowSize(THROW_POWER_BAR_SIZE);
    ImGui::Begin("##PowerBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImVec4 barColor = ImVec4(1.0f - (m_throwPower / m_maxThrowPower), (m_throwPower / m_maxThrowPower), 0.0f, 1.0f); // Gradient from red to green
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(m_throwPower / m_maxThrowPower, ImVec2(-1, 10.0f));
    ImGui::PopStyleColor();
    ImGui::End();

    // Render label "Power" below the bar
    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y - 20.0f));
    ImGui::SetNextWindowSize(ImVec2(100.0f, 10.0f));
    ImGui::Begin("##PowerLabel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Power");
    ImGui::End();

    // Pop all the style vars and colors
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

void PlayerController::CheckHealth() {
    auto* healthComponent = GetGameObject()->GetComponent<HealthComponent>();
    if (healthComponent)
    {
        if (m_godmode)
        {
            healthComponent->GodmodeHeal();
        }
        else
        {
            if (healthComponent->GetHealth() <= 0) SetAction(PlayerAction::DEAD);
        }
    }
}

void PlayerController::StartDeath() {
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    m_runtimeTimer.Stop(); // Stop the timer
    m_deathRuntime = m_runtimeTimer.Elapsed(); // Capture elapsed time once
    m_pAnimComponent->SetAnimPaused(true);
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    
    // Stop background music and play death music
    wolf::Audio::Stop("data/sounds/bgm_maze.wav");
    wolf::Audio::Stop("data/sounds/bgm_boss_theme.wav");
    wolf::Audio::Play("data/sounds/bgm_death.wav", 0.75f, 0.0f, 0.0f, true, 27.428f);
}

void PlayerController::RenderDeathScreen() {
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    // Step 1: Fade to Black
    if (!m_fadeComplete) {
        m_fadeOpacity += 0.01f;
        if (m_fadeOpacity >= 1.0f) {
            m_fadeOpacity = 1.0f;
            m_fadeComplete = true;
        }
    }

    // Render the fade overlay
    if (!m_fadeComplete) {
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(0, 0, 0, static_cast<int>(m_fadeOpacity * 255)));
    }

    // Load the black background image once
    static ImTextureID blackTextureID = nullptr;
    if (!blackTextureID) {
        blackTextureID = reinterpret_cast<void*>(wolf::TextureManager::CreateTexture("data/textures/black_background_1920x1080.png")->GetID());
    }

    // Display a scaled black background image
    ImVec2 overscaleSize(displaySize.x * 1.5f, displaySize.y * 1.25f);
    if (m_fadeComplete) {
        ImGui::SetNextWindowPos(ImVec2(-10, -10));
        ImGui::SetNextWindowSize(overscaleSize);
        ImGui::Begin("##BlackBackground", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
        ImGui::Image(blackTextureID, overscaleSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 0.75f));
        ImGui::End();
    }

    // Step 2: "You Died" message
    if (m_fadeComplete && !m_messageFadeComplete) {
        m_messageOpacity += 0.01f;
        if (m_messageOpacity >= 1.0f) {
            m_messageOpacity = 1.0f;
            m_messageFadeComplete = true;
        }
    }

    // Step 2: Enhanced "You Died" message
    if (m_messageFadeComplete || m_messageOpacity > 0.0f) {
        ImVec2 textPos((displaySize.x + 150.0f - (ImGui::CalcTextSize("You Died").x * 0.5f)) * 0.5f, displaySize.y * 0.4f);
        ImGui::SetNextWindowPos(textPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 100));
        ImGui::Begin("##GameOverMessage", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        
        // Add a glow effect using shadow text
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, m_messageOpacity * 0.5f));
        ImGui::SetWindowFontScale(2.8f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);  // Offset shadow vertically
        ImGui::Text("You Died");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);  // Reset position
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, m_messageOpacity)); // Bright red
        ImGui::Text("You Died");
        ImGui::PopStyleColor(2);
        ImGui::End();
    }

    // Step 3: Runtime display
    if (m_messageFadeComplete && !m_runtimeFadeComplete) {
        m_runtimeOpacity += 0.01f;
        if (m_runtimeOpacity >= 1.0f) {
            m_runtimeOpacity = 1.0f;
            m_runtimeFadeComplete = true;
        }
    }

    // Step 3: Enhanced runtime display
    if (m_runtimeFadeComplete || m_runtimeOpacity > 0.0f) {
        ImVec2 runtimePos((displaySize.x) * 0.5f, displaySize.y * 0.5f);
        ImGui::SetNextWindowPos(runtimePos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 100));
        ImGui::Begin("##RuntimeInfo", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

        // Add shadow text for a glowing effect
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, m_runtimeOpacity * 0.5f));
        ImGui::SetWindowFontScale(1.8f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        ImGui::Text("Run Time: %.2f seconds", m_deathRuntime);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, m_runtimeOpacity)); // White text
        ImGui::Text("Run Time: %.2f seconds", m_deathRuntime);
        ImGui::PopStyleColor(2);
        ImGui::End();
    }

    // Step 4: Options (Buttons)
    if (m_runtimeFadeComplete) {
        m_optionsOpacity += 0.01f;
        if (m_optionsOpacity > 1.0f) {
            m_optionsOpacity = 1.0f;
        }
    }

    if (m_optionsOpacity > 0.0f) {
        ImVec2 optionsPos((displaySize.x + 48.0f) * 0.5f, displaySize.y * 0.7f);
        ImGui::SetNextWindowPos(optionsPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(320, 160));
        ImGui::Begin("##DeathScreenOptions", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

        // Style adjustments for the buttons
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 15.0f));

        // Button colors with gradient effect
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, m_optionsOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.286f, 0.286f, 0.286f, m_optionsOpacity));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.14f, 0.14f, m_optionsOpacity));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.659f, 0.0f, m_optionsOpacity));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.0f, 0.0f, 0.0f, m_optionsOpacity * 0.6f));

        // Enable border and shadow for a polished look
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

        // "Return to Main Menu" button
        if (ImGui::Button("Return to Main Menu", ImVec2(240, 50))) {
            wolf::EventManager::EnqueueEvent(GameOverEvent(GameOverType::MAIN_MENU));
            ResetDeathScreenState();
        }

        ImGui::Spacing();

        // "Exit Game" button
        if (ImGui::Button("Exit Game", ImVec2(240, 50))) {
            wolf::EventManager::TriggerEvent(GameOverEvent(GameOverType::EXIT));
            ResetDeathScreenState();
        }

        // Pop all style changes
        ImGui::PopStyleVar(4); // Pop FrameRounding, FramePadding, ItemSpacing, and FrameBorderSize
        ImGui::PopStyleColor(5); // Pop Button, ButtonHovered, ButtonActive, Border, and BorderShadow
        ImGui::End();
    }
}

void PlayerController::ResetDeathScreenState() {
    // Reset fade animation variables
    m_fadeOpacity = 0.0f;
    m_fadeComplete = false;

    // Reset static black background flag
    m_blackBackgroundLoaded = false;

    // Reset "You Died" message fade variables
    m_messageOpacity = 0.0f;
    m_messageFadeComplete = false;

    // Reset runtime display fade variables
    m_runtimeOpacity = 0.0f;
    m_runtimeFadeComplete = false;

    // Reset options (buttons) fade variables
    m_optionsOpacity = 0.0f;
}