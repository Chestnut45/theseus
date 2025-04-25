//-----------------------------------------------------------------------------
// File: SnakeController.cpp
// Original Author:	Youssef Ashraf
// Modifications: Nguyễn Minh Nhật, D'Anyil Landry
// Controls snake attacks & behaviours.
//-----------------------------------------------------------------------------

#include <SnakeController.h>
#include <PlayerController.h>
#include <LabyrinthManager.h>
#include <GLShapesRenderer.h>
#include <DDACalculator.h>

#include <cassert>

// !- Aurora added this --!
#include <ItemDropCreator.h>

SnakeController::SnakeController()
{
    wolf::EventManager::AddListener<InfightingEvent, SnakeController, &SnakeController::HandleInfighting>(*this);
}

SnakeController::~SnakeController()
{
    wolf::EventManager::AddListener<InfightingEvent, SnakeController, &SnakeController::HandleInfighting>(*this);
}

void SnakeController::Init(const EnemyData& data)
{
    auto* pGameObject = GetGameObject();
    if (!pGameObject)
    {
        wolf::Error("LateInitialize failed: SnakeController not attached to GameObject!");
        return;
    }

    // Call base initialization
    EnemyController::Init();

    // Seed rng randomly
    m_RNG.SetSeed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    // Assign enemy data
    m_lungeRange = data.rangedRange;
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
        wolf::Warning("Snake " + std::to_string(pGameObject->GetID()) + " could not find VelocityComponent!");
    }

    // Set up snake-specific animations
    SetUpAnimations(data.animationInitFile);

    // Find and set the player as the target
    bool targetFound = false;
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        targetFound = true;
        break;
    }

    if (!targetFound)
    {
        wolf::Warning("Snake " + std::to_string(pGameObject->GetID()) + " did not find any player target!");
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
        break;
    }

    if (!pathfindingManagerFound)
    {
        wolf::Warning("SnakeController: No PathfindingManager found in the scene!");
    }

    bool navmeshfound = false;
    for (auto&& [entity, navmesh] : GetGameObject()->GetScene().Each<NavMeshComponent>())
    {
        m_pNavMeshComponent = &navmesh;
        navmeshfound = true;
        break;
    }
    if (!navmeshfound)
    {
        wolf::Warning("SnakeController: No navmesh found in the scene!");
    }

    // Initialize first state
    ChangeState(EnemyState::IDLE);
}


void SnakeController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_active || !m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    if (m_pTarget)
    {
        auto* targetHealth = m_pTarget->GetComponent<HealthComponent>();
        if (!targetHealth || targetHealth->GetHealth() <= 0) 
        {
            RevertToPlayerTarget();
        }
    }

    // Update the base class
    EnemyController::Update(delta);

    // Check if Snake is petrified
    StatusComponent* statusComponent = this->GetGameObject()->GetComponent<StatusComponent>();
    if(statusComponent != nullptr && statusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        if (m_state != EnemyState::PETRIFIED && m_state != EnemyState::DEATH)
        {
            ChangeState(EnemyState::PETRIFIED);
            return;
        }
    }
    else 
    {
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            ChangeState(EnemyState::IDLE);
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
            return;
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

void SnakeController::ChangeState(EnemyState newState)
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

void SnakeController::SetUpAnimations(const std::string& animationInitPath)
{
    auto* pGameObject = GetGameObject();
    
    // Check if the AnimatedSprite2D component exists
    if (pGameObject->HasAll<AnimatedSprite2D>())
    {
        pGameObject->DeleteComponent<AnimatedSprite2D>();
    }

    // Initialize the AnimatedSprite2D component
    m_pAnimComponent = &GetGameObject()->AddComponent<AnimatedSprite2D>(animationInitPath);
    m_pAnimComponent->SetLayer(7);
    m_pAnimComponent->SetLightingEnabled(false);
}

void SnakeController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform)
        return;

    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if (m_pTarget->HasAny<PlayerController>())
    {
        targetPosition.y -= 16.0f;
    }

    // Reset flag to always attempt using the navmesh first
    m_useNavMesh = true;
    
    // Try NavMesh pathfinding first
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
            
            // Add random variation so they don't stick on top of each other as much
            static wolf::RNG navRNG(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            m_navMeshPathUpdateTimer = navRNG.NextFloat(0.1f, 0.6f);
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
            if (distance <= 48.0f)
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
    
    // Fall back to grid-based pathfinding
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
            
            if (distance > 48.0f)
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
    
    // Fall back to direct distance checking
    FallbackToDistanceChecking();
}

// Fallback to direct distance checking if pathfinding fails
void SnakeController::FallbackToDistanceChecking()
{
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

    if (m_pTarget->HasAny<PlayerController>())
    {
        targetPosition.y -= 16.0f;
    }
    
    glm::vec2 direction = targetPosition - currentPosition;

    if (glm::length(direction) > 0.01f)
    {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
    }
    else
    {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
    }
}

void SnakeController::HandleIdleState(float delta)
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

void SnakeController::HandleProspectState(float delta)
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

void SnakeController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);

    if (m_slitherTimer.Elapsed() > m_nextSFXTime)
    {
        m_nextSFXTime = m_RNG.NextFloat(1.5f, 3.5f);
        m_slitherTimer.Restart();
        wolf::Audio::Play("data/sounds/sfx_snake_move.wav", 0.64f, m_RNG.NextInt(-10000, -5000));
    }

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

    // If player is within lunge range, transition delay expired, and melee timer expired, attack
    if (distanceToPlayer <= m_lungeRange)
    {
        if (m_transitionTimer.Elapsed() >= m_transitionDelay && m_meleeTimer <= 0.0f)
        {
            ChangeState(EnemyState::ATTACKING);
            return;
        }
    }
}

void SnakeController::HandleAttackingState(float delta)
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
        m_pAnimComponent->SetTint(glm::vec3(1.0f));

        // Check distance to player
        const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();

        // Determine if target's collider is active and a hurtbox
        auto* pCollider = m_pTarget->GetComponent<ColliderComponent>();
        const bool active = pCollider ? (pCollider->IsActive() && pCollider->IsHurtbox()) : false;
        
        const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
        const float distanceToTarget = glm::length(targetPosition - currentPosition);

        // Apply damage if target is within melee range and attack cooldown is over
        if (active && distanceToTarget <= m_meleeRange)
        {

            // Apply damage to the target
            auto* pTargetHealth = m_pTarget->GetComponent<HealthComponent>();
            if (pTargetHealth)
            {
                pTargetHealth->Damage(m_baseDamage);
            }

            // Random chance to poison the target
            if (m_RNG.NextInt(1, 10) <= 3)
            {
                auto* pTargetStatus = m_pTarget->GetComponent<StatusComponent>();
                if (pTargetStatus)
                {
                    pTargetStatus->AddStatusEffect(StatusComponent::StatusEffectType::POISONED, m_RNG.NextFloat(3.0f, 5.0f));
                }
            }

            // Apply strong knockback to the target
            auto* pTargetVelocity = m_pTarget->GetComponent<VelocityComponent>();
            if (pTargetVelocity)
            {
                // Calculate knockback direction and amplify the push
                glm::vec2 knockbackDirection = glm::normalize(targetPosition - currentPosition);
                float knockbackStrength = 800.0f;
                pTargetVelocity->ApplyKnockback(knockbackDirection, knockbackStrength);
            }

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

void SnakeController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));  
}

void SnakeController::HandleStunnedState(float delta)
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

void SnakeController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName = "";
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    switch (m_state)
    {
        case EnemyState::ATTACKING:
        {
            if(m_state == EnemyState::ATTACKING)
            {
                glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
                const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

                if (m_pTarget->HasAny<PlayerController>())
                {
                    targetPosition.y -= 16.0f;
                }

                const glm::vec2 vectorToTarget = targetPosition - currentPosition;
                const float distanceToTarget = glm::length(vectorToTarget);

                if (fabs(vectorToTarget.x) > fabs(vectorToTarget.y))
                {
                    animationName = (vectorToTarget.x > 0.0f) ? "AttackEast" : "AttackWest";
                }
                else
                {
                    animationName = (vectorToTarget.y > 0.0f) ? "AttackNorth" : "AttackSouth";
                }
            }
        }

        default:
        case EnemyState::PROSPECT:
        case EnemyState::CHASING:
        {
            if (glm::length(velocity) > 0.01f)
            {
                if (fabs(velocity.x) > fabs(velocity.y))
                {
                    animationName = (velocity.x > 0.0f) ? "East" : "West";
                }
                else
                {
                    animationName = (velocity.y > 0.0f) ? "North" : "South";
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

void SnakeController::HandleDeathState(float delta)
{
    m_pAnimComponent->SetAnimPaused(true);

    if (auto* pStatus = GetGameObject()->GetComponent<StatusComponent>())
    {
        // Only reset effects if not petrified
        if (!pStatus->IsStatusEffectActive(StatusComponent::PETRIFIED)) m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    }

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
            if(collider)
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
            std::vector<wolf::GameObject*> pItemDrops = ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/loot/snake_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            
            // Apply slight offset to drops
            for (auto& pItem : pItemDrops) {
                VelocityComponent* pItemVel = pItem->GetComponent<VelocityComponent>();
                if (pItemVel) {
                    pItemVel->ApplyKnockback(glm::vec2(1.0f, 0.0f), 10.0f);
                }
            }
            
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }  
}

void SnakeController::EnterAttackState()
{
    if (!m_pTarget)
    {
        ChangeState(EnemyState::IDLE);
        return;
    }

    // Lunge toward player
    glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    if (m_pTarget->HasAny<PlayerController>())
    {
        targetPosition.y -= 16.0f;
    }
    glm::vec2 lungeDir = targetPosition - currentPosition;
    if (glm::length(lungeDir) > 0.01f)
    {
        m_pVelocity->ApplyKnockback(glm::normalize(lungeDir), 1300.0f);
    }

    m_meleeWindupTimer = m_meleeWindupTime;
    wolf::Audio::Play("data/sounds/sfx_snake_hiss.wav", 0.32f, m_RNG.NextInt(-10000, 0));
}

void SnakeController::EnterChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Start();
    m_slitherTimer.Restart();
}

void SnakeController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    // Idle in a random direction
    static const char* dirs[] = {"North", "East", "South", "West"};
    static wolf::RNG idleRNG(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    m_pAnimComponent->SetAnimation(std::string(dirs[idleRNG.NextInt(0, 3)]));
}
void SnakeController::EnterPetrifiedState()
{
}

void SnakeController::EnterProspectState()
{
}

void SnakeController::EnterStunnedState()
{
    if (auto* pStatus = GetGameObject()->GetComponent<StatusComponent>())
    {
        // Only reset effects if not petrified
        if (!pStatus->IsStatusEffectActive(StatusComponent::PETRIFIED)) m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
    }
}
void SnakeController::EnterDeathState()
{
    SetEmote(EnemyEmote::NONE);
}

void SnakeController::ExitAttackState()
{
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
    
    m_meleeTimer = m_meleeCooldown;
    m_meleeWindupTimer = m_meleeWindupTime;
}

void SnakeController::ExitChasingState()
{
    m_transitionTimer.Reset();
    m_transitionTimer.Stop();
    m_transitionDelay = 0.0f;
    m_slitherTimer.Reset();
}

void SnakeController::ExitIdleState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
}

void SnakeController::ExitPetrifiedState()
{
    if (auto* pStatus = GetGameObject()->GetComponent<StatusComponent>())
    {
        // Only reset effects if not petrified
        if (!pStatus->IsStatusEffectActive(StatusComponent::PETRIFIED))
        {
            m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
            m_pAnimComponent->SetAnimPaused(false);
        }
    }
}

void SnakeController::ExitProspectState()
{
    m_targetDetectionTimer = m_RNG.NextFloat(0.2f, 0.4f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);
}

void SnakeController::ExitStunnedState()
{
    if (auto* pStatus = GetGameObject()->GetComponent<StatusComponent>())
    {
        // Only reset effects if not petrified
        if (!pStatus->IsStatusEffectActive(StatusComponent::PETRIFIED)) m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    }
    m_stunnedTimer = 0.0f;
}

void SnakeController::SetEmote(EnemyEmote p_emote)
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

bool SnakeController::IsTargetDetected()
{
    float distanceToTarget = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    return distanceToTarget <= m_detectionRange && IsTargetInLOS();
}

bool SnakeController::IsTargetInLOS()
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

glm::vec2 SnakeController::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 res = glm::vec2(
        (float)p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        (float)p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return res;
}

void SnakeController::HandleInfighting(const InfightingEvent& event)
{
    if (event.m_pVictim == GetGameObject())
    {
        // Ignore if already attacking this enemy
        if (m_pTarget == event.m_pAttacker) return;
        
        m_pTarget = event.m_pAttacker;
        ChangeState(EnemyState::CHASING);
    }
}

void SnakeController::RevertToPlayerTarget()
{
    for (auto&& [entity, playerController] : GetGameObject()->GetScene().Each<PlayerController>())
    {
        m_pTarget = playerController.GetGameObject();
        return;
    }

    m_pTarget = nullptr;
}