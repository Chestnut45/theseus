//-----------------------------------------------------------------------------
// File: GorgonController.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls gorgon attacks & behaviours.
//-----------------------------------------------------------------------------

#include "GorgonController.h"
#include "PlayerController.h"
#include "HarpyController.h"
#include "LabyrinthManager.h"
#include "../GLShapesRenderer.h"
#include "../DDACalculator.h"

#include <cassert>

GorgonController::GorgonController()
{
    wolf::EventManager::AddListener<InfightingEvent, GorgonController, &GorgonController::HandleInfighting>(*this);

}   

GorgonController::~GorgonController()
{
    wolf::EventManager::RemoveListener<InfightingEvent, GorgonController, &GorgonController::HandleInfighting>(*this);

}

void GorgonController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: GorgonController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);

    // Assign enemy data
    m_meleeRange = data.meleeRange;
    m_rangedRange = data.rangedRange;
    m_rangedCooldown = data.rangedCooldown;
    m_rangedWindupTime = data.rangedWindup;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);

    // Set attack timer
    m_rangedTimer = m_rangedCooldown;

    // Get required components and log their initialization
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity)
    {
        wolf::Warning("Gorgon " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up Gorgon-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    bool targetFound = false;
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        m_pTargetStatusComponent = m_pTarget->GetComponent<StatusComponent>();
        targetFound = true;
        break;  // Assume there's only one player
    }

    if (!targetFound)
    {
        wolf::Warning("Gorgon " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
    }

    // Init emotes object
    m_pEmoteObj = &pGameObject->GetScene().CreateObject2D();
    pGameObject->AddChild(*m_pEmoteObj);
    wolf::Transform2D* transform = m_pEmoteObj->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(-8.0f, 8.0f));

    // Init attack state members
    m_rangedWindupTimer = m_rangedWindupTime;

    // Add emotes spritesheet
    AnimatedSprite2D* emotesSpritesheet = &m_pEmoteObj->AddComponent<AnimatedSprite2D>("data/emotes_anim_init.yaml");
    emotesSpritesheet->SetAnimPaused(true);
    emotesSpritesheet->SetOriginToCenterOfFrame();

    // Initialise emotes-related variables
    m_fEmoteTimer = EMOTE_TIME;

    // Find the PathfindingManager in the scene
    bool pathfindingManagerFound = false;
    for (auto&& [entity, pathfindingManager] : GetGameObject()->GetScene().Each<PathfindingManager>())
    {
        m_pPathfindingManager = &pathfindingManager;
        pathfindingManagerFound = true;
        break; // there's only one PathfindingManager in the scene
    }

    if (!pathfindingManagerFound)
    {
        wolf::Warning("GorgonController: No PathfindingManager found in the scene!");
    }
    
    // Register with blackboard
    g_blackboard.RegisterEnemy(GetGameObject()->GetID());
    
    // Setup behavior tree
    SetupCombatBehaviorTree();
}


void GorgonController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    if (m_pTarget)
    {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) 
        {
            // wolf::Warning("BLUD CAN'T FIND A TARGET");
            RevertToPlayerTarget();
        }
    }
    
    
    // Update the base class
    EnemyController::Update(delta);

    // Optimize AI updates - only run on a timer
    static std::map<int, float> s_aiUpdateTimers;
    int myID = GetGameObject()->GetID();
    
    if (s_aiUpdateTimers.find(myID) == s_aiUpdateTimers.end()) {
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    s_aiUpdateTimers[myID] += delta;
    
    // Run expensive AI operations at most every 200ms
    bool shouldUpdateAI = (s_aiUpdateTimers[myID] > 0.2f);
    if (shouldUpdateAI) {
        // Evaluate strategy
        EvaluateStrategy();
        
        // Update blackboard
        UpdateBlackboard();
        
        // Reset timer
        s_aiUpdateTimers[myID] = 0.0f;
    }
    
    // Update behavior tree if in relevant states
    if (m_combatBehaviorTree && 
        (m_state == EnemyState::CHASING || 
         m_state == EnemyState::IDLE || 
         m_state == EnemyState::ATTACKING)) {
        m_combatBehaviorTree->Update(delta, &g_blackboard);
    }


    // Check if gorgon is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::PETRIFIED);
        return;
    }
    else 
    {
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            ChangeState(EnemyState::CHASING);
            return;
        }         
    }

    // Check if health is below or equal to 0 and not already dying, transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0 && m_state != EnemyState::DEATH)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
        return;
    }

    if(m_rangedTimer > 0.0f)
    {
        // Cooldown timer for next attack
        m_rangedTimer -= delta;
    }
    
    // Update based on the current state
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState(delta);
            break;
        case EnemyState::CHASING:
            HandleChasingState(delta);
            break;
        case EnemyState::PROSPECT:
            HandleProspectState(delta);
            break;
        case EnemyState::ATTACKING:
            HandleAttackingState(delta);
            break;
        case EnemyState::PETRIFIED:
            HandlePetrifiedState(delta);
            break;
        case EnemyState::STUNNED:
            HandleStunnedState(delta);
            break;
        case EnemyState::DEATH:
            HandleDeathState(delta);
            return;  // After calling HandleDeathState(), return immediately since the object is now deleted
    }

    // Update animations based on direction after handling movement
    UpdateAnimationBasedOnDirection();

    // Emoting
    if(m_fEmoteTimer > 0.0f)
    {
        m_fEmoteTimer -= delta;
    }
    else
    {
        // Clear previous emote
        if(m_emote != EnemyEmote::NONE)
        {
            SetEmote(EnemyEmote::NONE);
        }
    }
}

void GorgonController::ChangeState(EnemyState newState)
{
    // Exit old state
    switch (m_state)
    {
        case EnemyState::ATTACKING:
        {
            ExitAttackState();
            break;
        }
        case EnemyState::CHASING:
        {
            ExitChasingState();
            break;
        }
        case EnemyState::IDLE:
        {
            ExitIdleState();
            break;
        }
        case EnemyState::PETRIFIED:
        {
            ExitPetrifiedState();
            break;
        }
        case EnemyState::PROSPECT:
        {
            ExitProspectState();
            break;
        }
        case EnemyState::STUNNED:
        {
            ExitStunnedState();
            break;
        }
        default:
        {
            break;
        }
    }
    // Enter new state
        switch (newState)
    {
        case EnemyState::ATTACKING:
        {
            EnterAttackState();
            break;
        }
        case EnemyState::CHASING:
        {
            EnterChasingState();
            break;
        }
        case EnemyState::IDLE:
        {
            EnterIdleState();
            break;
        }
        case EnemyState::PETRIFIED:
        {
            EnterPetrifiedState();
            break;
        }
        case EnemyState::PROSPECT:
        {
            EnterProspectState();
            break;
        }
        case EnemyState::STUNNED:
        {
            EnterStunnedState();
            break;
        }
        case EnemyState::DEATH:
        {
            EnterDeathState();
            break;
        }
        default:
        {         
            break;
        }
    }
    m_last_state = m_state;
    m_state = newState;
}

void GorgonController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from Gorgon...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
    m_pAnimComponent->SetLayer(9);
}

void GorgonController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform || !m_pPathfindingManager)
        return;

    auto& pathData = m_pPathfindingManager->GetPathData(GetGameObject());

    if (pathData.path.empty())
    {
        FallbackToDirectMovement(delta);
        return;
    }

    glm::ivec2 nextTile = pathData.path.front();
    glm::vec2 nextTileWorldPos = m_pPathfindingManager->GetLabyrinthManager()->GetWorldPosition(nextTile) + glm::vec2(48.0f, 48.0f);
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 direction = nextTileWorldPos - currentPosition;

    if (glm::length(direction) > 0.5f)
    {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    }
    else
    {
        // Advance to the next tile
        pathData.path.erase(pathData.path.begin());
    }
}

// Fallback to direct movement if pathfinding fails
void GorgonController::FallbackToDirectMovement(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Gorgon
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

    // Calculate direction vector
    glm::vec2 direction = targetPosition - currentPosition;

    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    } else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void GorgonController::HandleIdleState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
            return;
        }

        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
        }
    }
    else
    {
        m_targetDetectionTimer -= delta;
    }
}

void GorgonController::HandleProspectState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {    
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING);
            return; 
        }
        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
        }
    }
    // Else, do thing
    else
    {
        m_targetDetectionTimer -= delta;
        // If pondering done, move
        if (m_prospectStandingCounter <= 0.0f)
        {
            // If moving done, decide next action
            if(m_prospectCounter <= 0)
            {          
                    // Roll for action
                    float rng = m_RNG.NextInt(1, 100);
                    // If larger than 10, prospect
                    if(rng > 10)
                    {
                        
                        m_prospectCounter = m_RNG.NextInt(1, 3);
                        glm::vec2 direction = glm::normalize(glm::vec2(m_RNG.NextInt(-100, 100), m_RNG.NextInt(-100, 100)));
                        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                    }

                    // Else, idle
                    else
                    {
                        ChangeState(EnemyState::IDLE);
                        return;
                    }
                    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);                  
            }
            else
            {
                m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));
                m_prospectCounter -= delta;
            }
        }
        else
        {
        m_prospectStandingCounter -= delta;
        }
    }
}

//-----------------------------------------------------------------------------
// Modified HandleChasingState to integrate with behavior tree
//-----------------------------------------------------------------------------
void GorgonController::HandleChasingState(float delta)
{
    // If we're repositioning, handle that movement specially
    if (m_isRepositioning) {
        MoveToOptimalPosition(delta);
        return;
    }
    
    // Standard movement
    MoveTowardsTarget(delta);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToTarget = glm::length(targetPosition - currentPosition);

    // If player is out of detection range or petrified, change state
    if (distanceToTarget > m_detectionRange || 
        (m_pTargetStatusComponent && m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)))
    {
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);
        return;
    }

    // If target is within ranged range, check if we can attack
    if (CanPerformGazeAttack()) {
        // Choose attack type based on blackboard strategy
        PrepareGazeAttack();
        ChangeState(EnemyState::ATTACKING);
        return;
    }
}

//-----------------------------------------------------------------------------
// Modified HandleAttackingState to integrate with behavior tree
//-----------------------------------------------------------------------------
void GorgonController::HandleAttackingState(float delta)
{
    if (!m_pTarget)
    {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // If winding up attack
    if(m_rangedWindupTimer > 0.0f)
    {
        m_rangedWindupTimer -= delta;
        
        // Brighten sprite to indicate attack
        if(m_pAnimComponent != nullptr)
        {
            glm::vec3 currentTint = m_pAnimComponent->GetTint();
            glm::vec3 nextTint = currentTint + glm::vec3(delta / (m_rangedWindupTime * 0.5f));
            m_pAnimComponent->SetTint(nextTint);
        }
        
        // Variable behavior based on attack type
        switch (m_currentGazeAttackType) {
            case GazeAttackType::QUICK:
                // Quick attack can have slight movement during windup
                if (m_rangedWindupTimer > m_rangedWindupTime * 0.5f) {
                    // Slight tracking movement during first half of windup
                    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
                    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
                    m_pVelocity->SetVelocity(direction * m_chaseSpeed * 0.2f);
                } else {
                    // Stop in place for the second half
                    m_pVelocity->SetVelocity(glm::vec2(0.0f));
                }
                break;
                
            case GazeAttackType::SUSTAINED:
                // Sustained attack requires complete stillness
                m_pVelocity->SetVelocity(glm::vec2(0.0f));
                break;
                
            case GazeAttackType::AREA:
                // Area attack can have a small amount of movement
                if (m_rangedWindupTimer > m_rangedWindupTime * 0.7f) {
                    // Very slight movement in first part of windup
                    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
                    glm::vec2 direction = glm::normalize(targetPosition - currentPosition);
                    m_pVelocity->SetVelocity(direction * m_chaseSpeed * 0.1f);
                } else {
                    // Stop for the final part
                    m_pVelocity->SetVelocity(glm::vec2(0.0f));
                }
                break;
        }
    }
    
    // Else, strike
    else
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint

        // Determine if target's collider is active and a hurtbox
        auto* pCollider = m_pTarget->GetComponent<ColliderComponent>();
        const bool active = pCollider ? (pCollider->IsActive() && pCollider->IsHurtbox()) : false;

        // If target is in line of sight, petrify target
        if(active && m_pTargetStatusComponent && IsTargetInLOS())
        {
            // Apply petrification with duration based on attack type
            float petrificationDuration = 5.0f; // Default duration
            
            switch (m_currentGazeAttackType) {
                case GazeAttackType::QUICK:
                    petrificationDuration = 3.0f; // Shorter duration
                    break;
                    
                case GazeAttackType::SUSTAINED:
                    petrificationDuration = 7.0f; // Longer duration
                    break;
                    
                case GazeAttackType::AREA:
                    petrificationDuration = 5.0f; // Standard duration
                    
                    // For area attack, check for other enemies nearby to help coordinate
                    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<GorgonController>()) {
                        if (controller.GetGameObject() != GetGameObject()) {
                            // Signal to other gorgons via blackboard that petrification succeeded
                            g_blackboard.SetBool("petrificationSuccess", true);
                            g_blackboard.SetInt("lastPetrifierID", GetGameObject()->GetID());
                            break;
                        }
                    }
                    break;
            }
            
            m_pTargetStatusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, petrificationDuration);
            
            // Record success for future strategy adjustment
            m_attackSuccessTimer = petrificationDuration;
            g_blackboard.SetFloat("lastPetrificationTime", m_attackSuccessTimer);
        }
        
        // Set cooldown based on attack type
        switch (m_currentGazeAttackType) {
            case GazeAttackType::QUICK:
                m_rangedTimer = m_rangedCooldown * 0.8f; // Shorter cooldown for quick attacks
                break;
                
            case GazeAttackType::SUSTAINED:
                m_rangedTimer = m_rangedCooldown * 1.2f; // Longer cooldown for sustained attacks
                break;
                
            case GazeAttackType::AREA:
                m_rangedTimer = m_rangedCooldown; // Standard cooldown for area attacks
                break;
        }
        
        ChangeState(EnemyState::CHASING);
        return;
    }

    m_curentCrosshairColour.g -= delta * (1.0f / m_rangedCooldown);
    RenderIndicator();
}

void GorgonController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));    
}

void GorgonController::HandleStunnedState(float delta)
{
    if(m_stunnedTimer >= m_stunnedTime)
    {
        if
        (IsTargetDetected())
        {
            if
            (
            m_last_state != EnemyState::CHASING     &&
            m_last_state != EnemyState::ATTACKING
            )
            {
                SetEmote(EnemyEmote::EXCLAMATION);
            }
            ChangeState(EnemyState::CHASING);
            return;
        }
        
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);
        return;
    }
    else
    {
        //m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
    }
}

void GorgonController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName = "";

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Gorgon is moving
    switch (m_state)
    {
        case EnemyState::ATTACKING:
        {
            if(m_state == EnemyState::ATTACKING)
            {
                const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
                const glm::vec2 vectorToTarget = targetPosition - currentPosition;
                const float distanceToTarget = glm::length(vectorToTarget);

                if (fabs(vectorToTarget.x) > fabs(vectorToTarget.y))
                {
                    // Moving left or right
                    animationName = (vectorToTarget.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    // Moving up or down
                    animationName = (vectorToTarget.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
        }

        case EnemyState::CHASING:
        {
            if (glm::length(velocity) > 0.01f)  // Ensure the velocity is not zero
            {
                // Check if the movement is more along the X or Y axis
                if (fabs(velocity.x) > fabs(velocity.y))
                {
                    // Moving left or right
                    animationName = (velocity.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    // Moving up or down
                    animationName = (velocity.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
            break;
        }

        case EnemyState::PROSPECT:
        {
            if (glm::length(velocity) > 0.01f)  // Ensure the velocity is not zero
            {
                // Check if the movement is more along the X or Y axis
                if (fabs(velocity.x) > fabs(velocity.y))
                {
                    // Moving left or right
                    animationName = (velocity.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    // Moving up or down
                    animationName = (velocity.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
            break;
        }

        default:
        {
            if (glm::length(velocity) > 0.01f)  // Ensure the velocity is not zero
            {
                // Check if the movement is more along the X or Y axis
                if (fabs(velocity.x) > fabs(velocity.y))
                {
                    // Moving left or right
                    animationName = (velocity.x > 0.0f) ? "StandEast" : "StandWest";
                }
                else
                {
                    // Moving up or down
                    animationName = (velocity.y > 0.0f) ? "StandNorth" : "StandSouth";
                }
            }
            break;
        }
    }

    // Check if the animation needs to be changed
    if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName)
    {
        m_pAnimComponent->SetAnimation(animationName);
        m_pAnimComponent->SetOriginToCenterOfFrame();
    }
}

void GorgonController::HandleDeathState(float delta)
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
                collider->SetIgnoreTag(m_uiPlayerGOId);
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
            // Spawn some loot
            std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            
            // Harpies can be inside of the walls so we need to push the loot out. To do that,
            // we get the loot item's velocity component
            for (auto& pItem : pItemDrops) {
                VelocityComponent* pItemVel = pItem->GetComponent<VelocityComponent>();
                if (pItemVel) {
                    // And gently push it in a random direction, which signals a collision in the ColliderManager
                    // that caluclates which direction the item should ACTUALLY be pushed in to get it out of the
                    // wall
                    pItemVel->ApplyKnockback(glm::vec2(1.0f, 0.0f), 10.0f);
                }
                ColliderComponent* pItemCollider = pItem->GetComponent<ColliderComponent>();
                if (pItemCollider)
                {
                    // Disable the collider after knockback
                    pItemCollider->SetActive(false);
                }
            }
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }
}

void GorgonController::EnterAttackState()
{
    m_IsRenderingAttackIndicator = true;
    m_crosshairOffset = glm::vec2(
        m_RNG.NextFloat(-4.0f, 4.0f),
        m_RNG.NextFloat(-4.0f, 4.0f)
        );
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void GorgonController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}

void GorgonController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}

void GorgonController::EnterPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::MULTITEX_PETRIFIED);
}

void GorgonController::EnterProspectState()
{
}

void GorgonController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}

void GorgonController::EnterDeathState()
{
    m_IsRenderingAttackIndicator = false;
    SetEmote(EnemyEmote::NONE);
}

void GorgonController::ExitAttackState()
{   
    m_curentCrosshairColour = CROSSHAIR_COLOUR;

    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }

    m_rangedTimer = m_rangedCooldown;
    m_rangedWindupTimer = m_rangedWindupTime;
}

void GorgonController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
}

void GorgonController::ExitIdleState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
}

void GorgonController::ExitPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void GorgonController::ExitProspectState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);
}

void GorgonController::ExitStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void GorgonController::SetEmote(EnemyEmote p_emote)
{
    // std::cout << "GorgonController - p_emote: " << p_emote << std::endl;
    m_fEmoteTimer = EMOTE_TIME;
    switch(p_emote)
    {
        case EnemyEmote::EXCLAMATION:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Exclamation");
            break;
        }

        case EnemyEmote::QUESTION:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("Question");
            break;
        }
        case EnemyEmote::NONE:
        {
            m_pEmoteObj->GetComponent<AnimatedSprite2D>()->SetAnimation("None");
            break;
        }
    }

    m_emote = p_emote;
}

bool GorgonController::IsTargetDetected()
{
    float distanceToTarget = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    if(
        m_pTargetStatusComponent != nullptr                                                             &&  // Target status component not null
        distanceToTarget <= m_detectionRange                                                            &&  // Target in detection range
        !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)   &&  // Target not already petrified
        IsTargetInLOS()                                                                                     // Target in line of sight
        )
    {  
        return true;
    }
    return false;
}

bool GorgonController::IsTargetInLOS()
{
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if(DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos) == targetPos)
    {
        return true;
    }

    return false;
    
}

void GorgonController::RenderIndicator()
{
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endPos = DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    glm::vec4 colour = this->m_curentCrosshairColour;
    
    DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    GLShapesRenderer::GetInstance()->AddLine(
                                            {thisPos.x, thisPos.y, colour.r, colour.g, colour.b, colour.a},
                                            {endPos.x, endPos.y, colour.r, colour.g, colour.b, colour.a}
                                            );
}

void GorgonController::HandleInfighting(const InfightingEvent& event)
{
    if (event.m_pVictim == GetGameObject()) // This Gorgon got hit
    {
        
        // Ignore if already attacking this enemy
        if (m_pTarget == event.m_pAttacker) return;

        // **Switch target to the attacker and start fighting back**
        m_pTarget = event.m_pAttacker;
        m_pTargetStatusComponent = m_pTarget->GetComponent<StatusComponent>();
        if (!m_pTargetStatusComponent)
        {
            // wolf::Warning("thats crazy lil bro, you got no status component? Me personally, i wouldn't allow that");
        }
        ChangeState(EnemyState::CHASING);
        // wolf::Log("Gorgon " + std::to_string(GetGameObject()->GetID()) + 
        //                 " is now fighting " + std::to_string(m_pTarget->GetID()));
    }
}


void GorgonController::RevertToPlayerTarget()
{
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        m_pTargetStatusComponent = m_pTarget->GetComponent<StatusComponent>();
        return;
    }

    // If no player found, log a warning
    // wolf::Warning("BLUD CAN'T FIND A TARGET");
    m_pTarget = nullptr; // No valid target
}

//-----------------------------------------------------------------------------
// SetupCombatBehaviorTree - Creates the behavior tree for Gorgon combat
//-----------------------------------------------------------------------------
void GorgonController::SetupCombatBehaviorTree() {
    // Create the root selector
    auto root = std::make_unique<Selector>();
    
    // === REPOSITIONING SEQUENCE ===
    auto repositioningSequence = std::make_unique<Sequence>();
    
    // Condition: Check if we should reposition
    repositioningSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            // No need to reposition during certain states
            if (m_state == EnemyState::ATTACKING || 
                m_state == EnemyState::STUNNED || 
                m_state == EnemyState::PETRIFIED ||
                m_state == EnemyState::DEATH ||
                m_isRepositioning) {
                return false;
            }
            
            // Update reposition timer
            static float s_lastUpdate = 0.0f;
            m_repositionTimer += 0.1f - s_lastUpdate;
            s_lastUpdate = 0.1f;
            
            // Check if it's time to consider repositioning
            if (m_repositionTimer < m_repositionDelay) {
                return false;
            }
            
            // Reset timer
            m_repositionTimer = 0.0f;
            
            // Defensive strategy repositions more frequently
            Strategy currentStrategy = g_blackboard.GetStrategy();
            if (currentStrategy == Strategy::DEFENSIVE) {
                m_repositionDelay = 4.0f;
            } else {
                m_repositionDelay = 7.0f;
            }
            
            // Random chance to reposition (reduced frequency)
            if (m_RNG.NextFloat(0.0f, 1.0f) > 0.3f) {
                return false;
            }
            
            // Get distance to target
            float distanceToTarget = GetDistanceToTarget();
            
            // If not in good attack range, consider repositioning
            return (distanceToTarget < m_rangedRange * 0.7f || distanceToTarget > m_rangedRange * 1.3f);
        }
    ));
    
    // Action: Find and move to optimal position
    repositioningSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (!m_isRepositioning) {
                // Calculate target position
                m_targetPosition = GetOptimalAttackPosition();
                m_isRepositioning = true;
                
                // Visual feedback
                SetEmote(EnemyEmote::QUESTION);
            }
            
            // Move towards target position
            MoveToOptimalPosition(delta);
            
            // Check if we've reached the position
            glm::vec2 currentPos = m_pTransform->GetGlobalPosition();
            float distanceToTarget = glm::length(m_targetPosition - currentPos);
            
            if (distanceToTarget < 10.0f) {
                m_isRepositioning = false;
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add repositioning sequence to root
    root->AddBehaviorNode(std::move(repositioningSequence));
    
    // === ATTACK SEQUENCE ===
    auto attackSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is in range and we can attack
    attackSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            return CanPerformGazeAttack();
        }
    ));
    
    // Action: Perform gaze attack
    attackSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            // If not already attacking, start attack
            if (m_state != EnemyState::ATTACKING) {
                PrepareGazeAttack();
                ChangeState(EnemyState::ATTACKING);
                
                // Set emote to indicate attack
                SetEmote(EnemyEmote::EXCLAMATION);
                return BehaviorNode::Status::RUNNING;
            }
            
            // Check if attack is complete
            if (m_state != EnemyState::ATTACKING) {
                return BehaviorNode::Status::SUCCESS;
            }
            
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add attack sequence to root
    root->AddBehaviorNode(std::move(attackSequence));
    
    // === CHASE SEQUENCE ===
    auto chaseSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is detected but not in attack range
    chaseSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() { 
            if (!m_pTarget || !m_pTargetStatusComponent) return false;
            
            // Don't chase if player is petrified
            if (m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)) {
                return false;
            }
            
            // Check distance to target
            float distanceToTarget = GetDistanceToTarget();
            
            // If target is in sight but outside optimal attack range, chase
            return IsTargetInLOS() && distanceToTarget > m_rangedRange;
        }
    ));
    
    // Action: Move towards player
    chaseSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (m_state != EnemyState::CHASING) {
                ChangeState(EnemyState::CHASING);
            }
            
            // Standard chase behavior is handled in HandleChasingState
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add chase sequence to root
    root->AddBehaviorNode(std::move(chaseSequence));
    
    // === PROSPECT SEQUENCE ===
    auto prospectSequence = std::make_unique<Sequence>();
    
    // Condition: Check if player is not visible or is petrified
    prospectSequence->AddBehaviorNode(std::make_unique<ConditionNode>(
        [this]() {
            if (!m_pTarget || !m_pTargetStatusComponent) return true;
            
            // If player is petrified, prospect around
            if (m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)) {
                return true;
            }
            
            // If player is not in line of sight, prospect
            if (!IsTargetInLOS()) {
                return true;
            }
            
            // If player is too far away, prospect
            float distanceToTarget = GetDistanceToTarget();
            return distanceToTarget > m_detectionRange;
        }
    ));
    
    // Action: Enter prospect state
    prospectSequence->AddBehaviorNode(std::make_unique<ActionNode>(
        [this](float delta) {
            if (m_state != EnemyState::PROSPECT) {
                ChangeState(EnemyState::PROSPECT);
            }
            
            // Standard prospect behavior is handled in HandleProspectState
            return BehaviorNode::Status::RUNNING;
        }
    ));
    
    // Add prospect sequence to root
    root->AddBehaviorNode(std::move(prospectSequence));
    
    // Create behavior tree with root
    m_combatBehaviorTree = std::make_unique<BehaviorTree>(std::move(root));
}

//-----------------------------------------------------------------------------
// EvaluateStrategy - Determines optimal strategy based on game state
//-----------------------------------------------------------------------------
void GorgonController::EvaluateStrategy() {
    if (!m_pTarget || !m_pHealth) return;
    
    // Cache values
    int myID = GetGameObject()->GetID();
    float selfHealth = m_pHealth->GetHealth();
    float healthPercentage = (selfHealth / m_pHealth->GetMaxHealth()) * 100.0f;
    
    // Get player health
    float playerHealth = 100.0f;
    auto* playerHealthComp = m_pTarget->GetComponent<HealthComponent>();
    if (playerHealthComp) {
        playerHealth = playerHealthComp->GetHealth();
    }
    
    // Count nearby gorgons
    int nearbyGorgons = 0;
    for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<GorgonController>()) {
        if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
            glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
            glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
            float distanceToEnemy = glm::length(enemyPos - selfPos);
            
            if (distanceToEnemy < m_detectionRange * 1.5f) {
                nearbyGorgons++;
            }
        }
    }
    
    // Count nearby enemies of any type
    int nearbyEnemies = nearbyGorgons;
    nearbyEnemies += g_blackboard.GetInt("nearbyHarpies");
    nearbyEnemies += g_blackboard.GetInt("nearbyEnemies"); // From Minitaur count
    
    // Strategy selection logic
    
    // 1. Low health - prioritize distance and safety
    if (healthPercentage < 30.0f) {
        g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        return;
    }
    
    // 2. Player low health - be more aggressive
    if (playerHealth < 20.0f) {
        g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        return;
    }
    
    // 3. Group tactics
    if (nearbyEnemies >= 1) {
        // When in a group, assign varied roles
        int roleSelector = myID % 3;
        switch (roleSelector) {
            case 0:
                g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
                break;
            case 1:
                g_blackboard.SetStrategy(Strategy::FLANKING);
                break;
            case 2:
                g_blackboard.SetStrategy(Strategy::DEFENSIVE);
                break;
        }
    } else {
        // Solo gorgon behavior
        auto* playerController = m_pTarget->GetComponent<PlayerController>();
        bool playerIsAttacking = false;
        
        if (playerController) {
            playerIsAttacking = playerController->GetPlayerAction() == PlayerController::PlayerAction::ATTACKING;
        }
        
        // If player is attacking or nearby, be more defensive
        if (playerIsAttacking || GetDistanceToTarget() < m_rangedRange * 0.7f) {
            g_blackboard.SetStrategy(Strategy::DEFENSIVE);
        } else {
            // Otherwise, default to aggressive
            g_blackboard.SetStrategy(Strategy::AGGRESSIVE);
        }
    }
}

//-----------------------------------------------------------------------------
// UpdateBlackboard - Updates shared information in the blackboard
//-----------------------------------------------------------------------------
void GorgonController::UpdateBlackboard() {
    if (!m_pTarget || !m_pTransform) return;
    
    // Calculate and store distance to player
    float distanceToPlayer = GetDistanceToTarget();
    g_blackboard.SetFloat("playerDistance", distanceToPlayer);
    
    // Update health data
    static float lastHealth = -1.0f;
    if (m_pHealth) {
        float currentHealth = m_pHealth->GetHealth();
        if (std::abs(currentHealth - lastHealth) > 0.1f) {
            g_blackboard.SetFloat("selfHealth", currentHealth);
            g_blackboard.SetFloat("selfHealthPercentage", 
                                 (currentHealth / m_pHealth->GetMaxHealth()) * 100.0f);
            lastHealth = currentHealth;
        }
    }
    
    // Count nearby gorgons periodically
    static float s_gorgonCountTimer = 0.0f;
    static int s_cachedNearbyGorgons = 0;
    
    s_gorgonCountTimer += 0.1f; // Approximate delta time
    if (s_gorgonCountTimer > 0.5f) {
        s_cachedNearbyGorgons = 0;
        for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<GorgonController>()) {
            if (controller.GetGameObject() != GetGameObject() && controller.GetTarget() == m_pTarget) {
                glm::vec2 enemyPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
                float distanceToEnemy = glm::length(enemyPos - selfPos);
                
                if (distanceToEnemy < m_detectionRange * 1.5f) {
                    s_cachedNearbyGorgons++;
                }
            }
        }
        g_blackboard.SetInt("nearbyGorgons", s_cachedNearbyGorgons);
        s_gorgonCountTimer = 0.0f;
    }
    
    // Track petrification success rate for strategy adjustment
    if (m_pTargetStatusComponent && 
        m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)) {
        m_attackSuccessTimer += 0.1f;
        g_blackboard.SetFloat("lastPetrificationTime", m_attackSuccessTimer);
    } else {
        m_attackSuccessTimer = 0.0f;
    }
}

//-----------------------------------------------------------------------------
// GetDistanceToTarget - Calculate distance to current target
//-----------------------------------------------------------------------------
float GorgonController::GetDistanceToTarget() const {
    if (!m_pTarget || !m_pTransform) return 9999.0f;
    
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    return glm::length(targetPosition - currentPosition);
}

//-----------------------------------------------------------------------------
// PrepareGazeAttack - Setup gaze attack based on current strategy
//-----------------------------------------------------------------------------
void GorgonController::PrepareGazeAttack() {
    // Choose attack type based on strategy
    Strategy currentStrategy = g_blackboard.GetStrategy();
    
    if (currentStrategy == Strategy::AGGRESSIVE) {
        // Quick attack - faster windup, shorter petrification
        m_currentGazeAttackType = GazeAttackType::QUICK;
        m_rangedWindupTimer = m_rangedWindupTime * 0.7f;
    } 
    else if (currentStrategy == Strategy::DEFENSIVE) {
        // Sustained attack - longer windup, longer petrification
        m_currentGazeAttackType = GazeAttackType::SUSTAINED;
        m_rangedWindupTimer = m_rangedWindupTime * 1.2f;
    }
    else if (currentStrategy == Strategy::FLANKING) {
        // Area attack - medium windup, used when flanking with other enemies
        m_currentGazeAttackType = GazeAttackType::AREA;
        m_rangedWindupTimer = m_rangedWindupTime;
    }
    else {
        // Default to quick attack
        m_currentGazeAttackType = GazeAttackType::QUICK;
        m_rangedWindupTimer = m_rangedWindupTime;
    }
    
    // Visual feedback for attack type
    switch (m_currentGazeAttackType) {
        case GazeAttackType::QUICK:
            m_curentCrosshairColour = glm::vec4(1.0f, 0.7f, 0.0f, 1.0f); // Orange
            break;
            
        case GazeAttackType::SUSTAINED:
            m_curentCrosshairColour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            break;
            
        case GazeAttackType::AREA:
            m_curentCrosshairColour = glm::vec4(0.7f, 0.0f, 0.7f, 1.0f); // Purple
            break;
    }
    
    m_IsRenderingAttackIndicator = true;
}

//-----------------------------------------------------------------------------
// CanPerformGazeAttack - Check if conditions are right for a gaze attack
//-----------------------------------------------------------------------------
bool GorgonController::CanPerformGazeAttack() {
    if (!m_pTarget || !m_pTransform || !m_pTargetStatusComponent) return false;
    
    // Basic conditions
    float distanceToTarget = GetDistanceToTarget();
    bool inRange = (distanceToTarget <= m_rangedRange);
    bool cooledDown = (m_rangedTimer <= 0.0f);
    bool targetNotPetrified = !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED);
    bool targetInSight = IsTargetInLOS();
    bool transitionReady = (m_transitionTimer.Elapsed() >= m_transitionDelay);
    
    // All conditions must be met
    return inRange && cooledDown && targetNotPetrified && targetInSight && transitionReady;
}

//-----------------------------------------------------------------------------
// GetOptimalAttackPosition - Find best position for gorgon to attack from
//-----------------------------------------------------------------------------
glm::vec2 GorgonController::GetOptimalAttackPosition() {
    if (!m_pTarget || !m_pTransform) {
        return m_pTransform->GetGlobalPosition();
    }
    
    glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 selfPos = m_pTransform->GetGlobalPosition();
    
    // Determine optimal attack distance based on strategy
    Strategy currentStrategy = g_blackboard.GetStrategy();
    float optimalDistance = m_rangedRange * 0.9f; // Default: just inside attack range
    
    if (currentStrategy == Strategy::DEFENSIVE) {
        optimalDistance = m_rangedRange * 0.95f; // Further when defensive
    } else if (currentStrategy == Strategy::AGGRESSIVE) {
        optimalDistance = m_rangedRange * 0.8f; // Closer when aggressive
    }
    
    // For group coordination, distribute positions around player
    int nearbyGorgons = g_blackboard.GetInt("nearbyGorgons");
    int myID = GetGameObject()->GetID();
    
    // Generate potential positions in a circle around player
    std::vector<glm::vec2> potentialPositions;
    int angleCount = 8; // Check 8 evenly distributed directions
    
    for (int i = 0; i < angleCount; i++) {
        float angle = (i * 2.0f * glm::pi<float>()) / angleCount;
        glm::vec2 direction(cos(angle), sin(angle));
        glm::vec2 position = playerPos + direction * optimalDistance;
        potentialPositions.push_back(position);
    }
    
    // Filter and score positions
    std::vector<std::pair<glm::vec2, float>> scoredPositions;
    
    // Get DDACalculator for wall checking
    DDACalculator* pDDA = DDACalculator::GetInstance();
    
    for (const auto& pos : potentialPositions) {
        // First use DDACalculator to check if this position is in a wall
        if (pDDA && pDDA->GetLabyrinthManager()) {
            LabyrinthManager* pLBMG = pDDA->GetLabyrinthManager();
            glm::ivec2 tilePos = pLBMG->GetTilePosition(pos);
            int tileID = pLBMG->GetTile(tilePos.x, tilePos.y);
            
            // Skip this position if it's in a wall tile
            if (tileID >= Tile::WallBottomLeft && tileID <= Tile::WallTop) {
                continue;
            }
        }
        
        // Position is not in a wall, continue with other checks
        if (IsPositionSafe(pos)) {
            float score = 0.0f;
            
            // Distance from current position (closer is better)
            float distanceFromCurrent = glm::length(pos - selfPos);
            score -= distanceFromCurrent * 0.1f;
            
            // Check if position avoids clustering with other gorgons
            bool isClear = true;
            for (auto&& [entity, controller] : GetGameObject()->GetScene().Each<GorgonController>()) {
                if (controller.GetGameObject()->GetID() != myID) {
                    glm::vec2 otherPos = controller.GetGameObject()->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                    float distToOther = glm::length(otherPos - pos);
                    
                    if (distToOther < 50.0f) {
                        isClear = false;
                        break;
                    }
                }
            }
            
            if (isClear) {
                score += 30.0f;
            }
            
            // Prefer positions aligned with group strategy
            if (currentStrategy == Strategy::FLANKING) {
                // When flanking, prefer positions not in player's field of view
                auto* playerController = m_pTarget->GetComponent<PlayerController>();
                if (playerController) {
                    glm::vec2 playerFacing = playerController->GetLastFacingDirectionVector();
                    glm::vec2 toPosition = glm::normalize(pos - playerPos);
                    float dotProduct = glm::dot(playerFacing, toPosition);
                    
                    // Negative dot product means we're behind player
                    if (dotProduct < 0) {
                        score += 25.0f;
                    }
                }
            }
            
            // For LOS attackers like Gorgons, positions with clear line of sight are critical
            glm::vec2 endpoint = pDDA->GetEndpoint(pos, playerPos);
            if (glm::length(endpoint - playerPos) < 0.1f) {
                // Clear line of sight to player from this position
                score += 50.0f;
            }
            
            scoredPositions.push_back({pos, score});
        }
    }
    
    // If no valid positions found, return current position
    if (scoredPositions.empty()) {
        return selfPos;
    }
    
    // Find position with highest score
    std::sort(scoredPositions.begin(), scoredPositions.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return scoredPositions[0].first;
}

//-----------------------------------------------------------------------------
// MoveToOptimalPosition - Move gorgon to target position
//-----------------------------------------------------------------------------
void GorgonController::MoveToOptimalPosition(float delta) {
    if (!m_pVelocity || !m_pTransform) return;
    
    glm::vec2 currentPos = m_pTransform->GetGlobalPosition();
    glm::vec2 toTarget = m_targetPosition - currentPos;
    float distanceToTarget = glm::length(toTarget);
    
    // Only move if we're not very close to the target position
    if (distanceToTarget > 10.0f) {
        glm::vec2 direction = toTarget / distanceToTarget; // Normalized direction
        
        // Apply smoothing - current velocity contributes 70%, new direction 30%
        glm::vec2 currentVelocity = m_pVelocity->GetVelocity();
        glm::vec2 targetVelocity = direction * m_chaseSpeed * 0.8f;
        
        // Blend velocities for smooth movement
        glm::vec2 blendedVelocity = currentVelocity * 0.7f + targetVelocity * 0.3f;
        
        // Scale down movement as we get closer to the target
        float slowdownFactor = std::min(1.0f, distanceToTarget / 50.0f);
        
        m_pVelocity->SetVelocity(blendedVelocity * slowdownFactor);
    } else {
        // We've arrived at the target position
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

//-----------------------------------------------------------------------------
// IsPositionSafe - Check if a position is valid for the gorgon
//-----------------------------------------------------------------------------
bool GorgonController::IsPositionSafe(const glm::vec2& position) {
    // Basic implementation - could be expanded with more checks
    
    // Check if position is too close to player (might be dangerous)
    if (m_pTarget) {
        glm::vec2 playerPos = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        float distToPlayer = glm::length(position - playerPos);
        
        // Too close to player might be dangerous
        if (distToPlayer < m_meleeRange * 1.5f) {
            return false;
        }
    }
    
    // Further checks could be added (e.g., proximity to other enemies, traps, etc.)
    return true;
}   

