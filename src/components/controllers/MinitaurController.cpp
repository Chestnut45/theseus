//-----------------------------------------------------------------------------
// File: MinitaurController.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật
// Controls minitaur attacks & behaviours.
//-----------------------------------------------------------------------------
#include <MinitaurController.h>
#include <PlayerController.h>
#include <LabyrinthManager.h>
#include <GLShapesRenderer.h>
#include <DDACalculator.h>

#include <cassert>

// !- Aurora added this --!
#include <ItemDropCreator.h>

MinitaurController::MinitaurController()
{
    wolf::EventManager::AddListener<InfightingEvent, MinitaurController, &MinitaurController::HandleInfighting>(*this);
}

MinitaurController::~MinitaurController()
{
    wolf::EventManager::AddListener<InfightingEvent, MinitaurController, &MinitaurController::HandleInfighting>(*this);
}

void MinitaurController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: MinitaurController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();

    // Assign enemy data
    m_meleeRange = data.meleeRange;
    m_meleeCooldown = data.attackCooldown;
    m_meleeWindupTime = data.meleeWindup;
    m_detectionRange = data.detectionRange;
    m_baseDamage = data.baseDamage;
    m_chaseSpeed = data.chaseSpeed;

    // Set attack timer
    m_meleeTimer = m_meleeCooldown;

    // Get required components and log their initialization
    m_pVelocity = GetGameObject()->GetComponent<VelocityComponent>();
    if (!m_pVelocity)
    {
        wolf::Warning("Minitaur " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up Minitaur-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    bool targetFound = false;
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        targetFound = true;
        break;  // Assume there's only one player
    }

    if (!targetFound)
    {
        wolf::Warning("Minitaur " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
    }

    // Init attack state members
    m_meleeWindupTimer = m_meleeWindupTime;

    // Init emotes object
    m_pEmoteObj = &pGameObject->GetScene().CreateObject2D();
    pGameObject->AddChild(*m_pEmoteObj);
    wolf::Transform2D* transform = m_pEmoteObj->GetComponent<wolf::Transform2D>();
    transform->SetPosition(glm::vec2(0.0f, 16.0f));

    // Add emotes spritesheet
    AnimatedSprite2D* emotesSpritesheet = &m_pEmoteObj->AddComponent<AnimatedSprite2D>("data/animations/emotes_anim_init.yaml");
    emotesSpritesheet->SetAnimPaused(true);
    emotesSpritesheet->SetLightingEnabled(false);
    emotesSpritesheet->SetLayer(100);

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
        wolf::Warning("MinitaurController: No PathfindingManager found in the scene!");
    }

    bool navmeshfound = false;
    for (auto&& [entity, navmesh] : GetGameObject()->GetScene().Each<NavMeshComponent>())
    {
        m_pNavMeshComponent = &navmesh;
        navmeshfound = true;
        break; // there's only one PathfindingManager in the scene
    }
    if (!navmeshfound)
    {
        wolf::Warning("MinitaurController: No navmesh found in the scene!");
    }
}


void MinitaurController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    if (m_pTarget)
    {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) 
        {
            // wolf::Warning("LIL BLUD CAN'T FIND A TARGET, SO HE'S SWITCHING BACK TO THE PLAYER");
            RevertToPlayerTarget();
        }
    }

    // Update the base class
    EnemyController::Update(delta);

    // Check if minitaur is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::PETRIFIED);
    }
    else 
    {
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            ChangeState(EnemyState::CHASING);
        }         
    }

    // Check if health is below or equal to 0 and not already dying, transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0 && m_state != EnemyState::DEATH)
    {
        // Switch to the DEATH state if the health is depleted
        ColliderComponent* collider = this->GetGameObject()->GetComponent<ColliderComponent>();
        collider->SetActive(false);
        ChangeState(EnemyState::DEATH);
        
        return;
    }

    if(m_meleeTimer > 0.0f)
    {
        // Cooldown timer for next attack
        m_meleeTimer -= delta;
    }

    // Update based on the current state
    switch (m_state)
    {
        case EnemyState::IDLE:
            HandleIdleState(delta);
            break;
        case EnemyState::PROSPECT:
            HandleProspectState(delta);
            break;
        case EnemyState::CHASING:
            HandleChasingState(delta);
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
        if(m_emote != EnemyEmote::NONE)
        {
            // Clear previous emote
            SetEmote(EnemyEmote::NONE);
        }
    }
}

void MinitaurController::ChangeState(EnemyState newState)
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

void MinitaurController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();  // Use DeleteComponent to remove the existing component
        wolf::Warning("Removed existing anim component from minitaur...");
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
    m_pAnimComponent->SetLayer(9);
    m_pAnimComponent->SetLightingEnabled(false);
}

void MinitaurController::RenderDebugPath()
{

}

void MinitaurController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform)
        return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    
    // STEP 1: Try NavMesh pathfinding first
    if (m_pNavMeshComponent && m_useNavMesh)
    {
        // Update path periodically
        if (m_navMeshPath.empty() || m_navMeshPathUpdateTimer <= 0.0f)
        {
            // Try full NavMesh pathfinding first
            m_navMeshPath = m_pNavMeshComponent->FindPath(currentPosition, targetPosition);
            
            // Validate the path
            if (m_navMeshPath.size() >= 2 && !m_pNavMeshComponent->IsPathValid(m_navMeshPath))
            {
                m_navMeshPath = m_pNavMeshComponent->CreateGridBasedPath(currentPosition, targetPosition);
                
                if (m_navMeshPath.size() < 2)
                {
                    m_useNavMesh = false;
                }
            }
            else if (m_navMeshPath.size() < 2)
            {
                m_useNavMesh = false;
            }
            
            m_navMeshPathUpdateTimer = 0.5f;
        }
        else
        {
            m_navMeshPathUpdateTimer -= delta;
        }
        
        // Follow NavMesh path if valid
        if (m_navMeshPath.size() >= 2)
        {
            glm::vec2 nextPoint = m_navMeshPath[1];
            glm::vec2 direction = nextPoint - currentPosition;
            float distance = glm::length(direction);
            
            // More generous tolerance for waypoint arrival
            if (distance <= 10.0f)
            {
                m_navMeshPath.erase(m_navMeshPath.begin());
            }
            else
            {
                direction = glm::normalize(direction);
                m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                return; // Successfully using NavMesh
            }
        }
    }
    
    // STEP 2: Fall back to grid-based pathfinding
    if (m_pPathfindingManager)
    {
        auto& pathData = m_pPathfindingManager->GetPathData(GetGameObject());
        
        // If we need a new grid path
        if (pathData.path.empty() || pathData.targetTile != glm::ivec2(targetPosition) / 
                                     (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE))
        {
            glm::ivec2 currentTile = glm::ivec2(currentPosition) / 
                                    (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
            glm::ivec2 targetTile = glm::ivec2(targetPosition) / 
                                   (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);
            
            
            // Explicitly request a new path
            std::vector<glm::ivec2> newPath = m_pPathfindingManager->FindPath(currentTile, targetTile);
            
            if (!newPath.empty())
            {
                pathData.path = newPath;
                pathData.targetTile = targetTile;
            }
        }
        
        if (!pathData.path.empty())
        {
            glm::ivec2 nextTile = pathData.path.front();
            glm::vec2 nextTileWorldPos = m_pPathfindingManager->GetLabyrinthManager()->GetWorldPosition(nextTile) + 
                                       glm::vec2(48.0f, 48.0f);
            glm::vec2 direction = nextTileWorldPos - currentPosition;
            float distance = glm::length(direction);
            
            if (distance > 0.5f)
            {
                direction = glm::normalize(direction);
                m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                return; // Successfully using grid pathfinding
            }
            else
            {
                // Advance to the next waypoint
                pathData.path.erase(pathData.path.begin());
                return;
            }
        }
    }
    
    // STEP 3: Fall back to direct distance checking
    FallbackToDistanceChecking();
}

// Fallback to direct distance checking if pathfinding fails
void MinitaurController::FallbackToDistanceChecking()
{
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 direction = targetPosition - currentPosition;

    if (glm::length(direction) > 0.01f)
    {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);

        // Debug: Print fallback movement
        // printf("Moving directly towards target: (%f, %f) with velocity (%f, %f)\n",
        //        targetPosition.x, targetPosition.y, direction.x, direction.y);
    }
    else
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void MinitaurController::HandleIdleState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
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

void MinitaurController::HandleProspectState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {    
        // If target detected, emote & chase
        if (IsTargetDetected())
        {
            SetEmote(EnemyEmote::EXCLAMATION);
            ChangeState(EnemyState::CHASING); 
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
                    m_prospectCounter = m_RNG.NextInt(100, 200);
                    glm::vec2 direction = glm::normalize(glm::vec2(m_RNG.NextInt(-100, 100), m_RNG.NextInt(-100, 100)));
                    m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                }

                // Else, change to idle
                else
                {
                    ChangeState(EnemyState::IDLE);
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

void MinitaurController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToPlayer = glm::length(targetPosition - currentPosition);

    // If player is out of detection range, emote & switch to prospect
    if (distanceToPlayer > m_detectionRange)
    {
        SetEmote(EnemyEmote::QUESTION);
        ChangeState(EnemyState::PROSPECT);      
        return;
    }

    // If player is within melee range, transition delay expired, and melee timer expired, attack
    if (distanceToPlayer <= m_meleeRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay && m_meleeTimer <= 0.0f)
        {
            ChangeState(EnemyState::ATTACKING);
            return;
        }
    }
}

void MinitaurController::HandleAttackingState(float delta)
{
    if (!m_pTarget)
    {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // If winding up attack
    if(m_meleeWindupTimer > 0.0f)
    {
        // Brighten sprite to indicate attack
        if(m_pAnimComponent != nullptr)
        {
            glm::vec3 nextTint = glm::vec3(glm::mix(1.0f, 2.0f, (m_meleeWindupTime - m_meleeWindupTimer) / m_meleeWindupTime));
            m_pAnimComponent->SetTint(nextTint);
        }

        m_meleeWindupTimer -= delta;
    }

    // Else, strike
    else
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f)); // Reset windup tint

        // Check distance to player
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        // Determine if target's collider is active and a hurtbox
        auto* pCollider = m_pTarget->GetComponent<ColliderComponent>();
        const bool active = pCollider ? (pCollider->IsActive() && pCollider->IsHurtbox()) : false;
        
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        const float distanceToPlayer = glm::length(targetPosition - currentPosition);

        // Apply damage if player is within melee range and attack cooldown is over
        if (active && distanceToPlayer <= m_meleeRange)
        {

            // Apply damage to the player
            auto* playerHealth = m_pTarget->GetComponent<HealthComponent>();
            if (playerHealth)
            {
                playerHealth->Damage(m_baseDamage);
            }

            // Apply strong knockback to the player
            auto* playerVelocity = m_pTarget->GetComponent<VelocityComponent>();
            if (playerVelocity)
            {
                // Calculate knockback direction and amplify the push
                glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                float knockbackStrength = 800.0f; // Amplified knockback strength
                playerVelocity->ApplyKnockback(knockbackDirection, knockbackStrength);
            }

            // Chain another attack
            // ChangeState(EnemyState::ATTACKING);
            ChangeState(EnemyState::CHASING);
            return;
        }
        else
        {
            // Return to chasing if player moves out of range
            ChangeState(EnemyState::CHASING);
            return;
        }
    }
}

void MinitaurController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));  
}

void MinitaurController::HandleStunnedState(float delta)
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
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
        m_stunnedTimer += delta;
    }
}

void MinitaurController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Minitaur is moving
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
    else
    {
        // If not moving, default to idle state based on the last direction
        animationName = "StandSouth";  // Modify as needed
    }

    // Check if the animation needs to be changed
    if (m_pAnimComponent->GetCurrentAnimation()->m_strName != animationName)
    {
        m_pAnimComponent->SetAnimation(animationName);
        m_pAnimComponent->SetOriginToCenterOfFrame();
    }
}

    void MinitaurController::HandleDeathState(float delta)
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
                // !-- Aurora added this --!
                // Spawn some loot
                std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/loot/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
                
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

void MinitaurController::EnterAttackState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
    m_meleeWindupTimer = m_meleeWindupTime;
}

void MinitaurController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
}

void MinitaurController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));
}
void MinitaurController::EnterPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::MULTITEX_PETRIFIED);
}

void MinitaurController::EnterProspectState()
{
}

void MinitaurController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}
void MinitaurController::EnterDeathState()
{
    SetEmote(EnemyEmote::NONE);
}

void MinitaurController::ExitAttackState()
{
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
    
    m_meleeTimer = m_meleeCooldown;
    m_meleeWindupTimer = m_meleeWindupTime;
}

void MinitaurController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = 0.0f;
}

void MinitaurController::ExitIdleState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
}

void MinitaurController::ExitPetrifiedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_pAnimComponent->SetAnimPaused(false);
}

void MinitaurController::ExitProspectState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);
}

void MinitaurController::ExitStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
}

void MinitaurController::SetEmote(EnemyEmote p_emote)
{
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

bool MinitaurController::IsTargetDetected()
{
    float distanceToTarget = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    if(
        distanceToTarget <= m_detectionRange    &&  // Target in detection range
        IsTargetInLOS()                             // Target in line of sight
        )
    {
        return true;
    }
    return false;
}

bool MinitaurController::IsTargetInLOS()
{
    glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    glm::vec2 endPos = DDACalculator::GetInstance()->GetEndpoint(thisPos, targetPos);
    if(endPos == targetPos)
    {
        return true;
    }
    return false;
}

glm::vec2 MinitaurController::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 res = glm::vec2(
        (float)p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        (float)p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return res;
}

void MinitaurController::HandleInfighting(const InfightingEvent& event)
{
    if (event.m_pVictim == GetGameObject()) // This Minitaur got hit
    {
            // Ignore if already attacking this enemy
            if (m_pTarget == event.m_pAttacker) return;

            // **Minitaur fights back once hit**
            m_pTarget = event.m_pAttacker;
            ChangeState(EnemyState::CHASING);

            // wolf::Log("Minitaur " + std::to_string(GetGameObject()->GetID()) + 
            //                 " is now fighting " + std::to_string(m_pTarget->GetID()));
    }
}

void MinitaurController::RevertToPlayerTarget()
{
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        return;
    }

    // If no player found, log a warning
    // wolf::Warning("BLUD CAN'T FIND A TARGET");
    m_pTarget = nullptr; // No valid target
}