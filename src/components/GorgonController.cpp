#include "GorgonController.h"
#include "PlayerController.h"
#include "LabyrinthManager.h"

#include <cassert>



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
}


void GorgonController::Update(float delta)
{
    // Ensure components and target are initialized before performing any updates
    if (!m_pTransform || !m_pVelocity || !m_pHealth || !m_pTarget)
        return;

    // Check if gorgon is petrified
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

    // Check if health is below or equal to 0 and transition to the DEATH state
    if (m_pHealth->GetHealth() <= 0)
    {
        // Switch to the DEATH state if the health is depleted
        ChangeState(EnemyState::DEATH);
    }
    
    // Update based on the current state
    switch (m_state)
    {
        case EnemyState::IDLE:
            // std::cout << "GorgonController - Idle" << std::endl;
            HandleIdleState(delta);
            break;
        case EnemyState::CHASING:
            // std::cout << "GorgonController - Chasing" << std::endl;
            HandleChasingState(delta);
            break;
        case EnemyState::PROSPECT:
            // std::cout << "GorgonController - Prospect" << std::endl;
            HandleProspectState(delta);
            break;
        case EnemyState::ATTACKING:
            // std::cout << "GorgonController - Attacking" << std::endl;
            HandleAttackingState(delta);
            break;
        case EnemyState::PETRIFIED:
            // std::cout << "GorgonController - Petrified" << std::endl;
            HandlePetrifiedState(delta);
            break;
        case EnemyState::STUNNED:
            // std::cout << "GorgonController - Stunned" << std::endl;
            HandleStunnedState(delta);
            break;
        case EnemyState::DEATH:
            // std::cout << "GorgonController - Death" << std::endl;
            HandleDeathState(delta);
            return;  // After calling HandleDeathState(), return immediately since the object is now deleted
    }

    // Update animations based on direction after handling movement
    UpdateAnimationBasedOnDirection();
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
        case EnemyState::IDLE:
        {
            EnterIdleState();
            break;
        }
        case EnemyState::STUNNED:
        {
            EnterStunnedState();
            break;
        }
        default:
        {         
            break;
        }
    }

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
}

void GorgonController::MoveTowardsTarget(float delta)
{
    if (!m_pTarget || !m_pVelocity || !m_pTransform) return;

    // Calculate the direction towards the player and move the Gorgon
    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();

    // Calculate direction vector
    glm::vec2 direction = targetPosition - currentPosition;

    // Log for debugging current position, target position, and distance
    // printf("Gorgon MoveTowardsTarget: Current Pos: (%f, %f), Target Pos: (%f, %f)\n", 
    //        currentPosition.x, currentPosition.y, targetPosition.x, targetPosition.y);

    if (glm::length(direction) > 0.01f) {
        direction = glm::normalize(direction);
        m_pVelocity->SetVelocity(direction * m_chaseSpeed);

        // Log the velocity being set
        // printf("Velocity Set: (%f, %f)\n", direction.x * m_chaseSpeed, direction.y * m_chaseSpeed);
    } else {
        m_pVelocity->SetVelocity(glm::vec2(0.0f));
        // printf("Velocity Stopped\n");
    }
}

void GorgonController::HandleIdleState(float delta)
{
    // If detection timer expired, perform detection check
    if(m_targetDetectionTimer <= 0.0f)
    {
        // If target detected, chase
        if (IsTargetDetected())
        {
            ChangeState(EnemyState::CHASING); 
        }

        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.4f, 0.8f);
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
        // If target detected, chase
        if (IsTargetDetected())
        {
            ChangeState(EnemyState::CHASING); 
        }
        // Else, reset detection timer
        else
        {
            m_targetDetectionTimer = m_RNG.NextFloat(0.4f, 0.8f);
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
                    // Roll for prospect
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

void GorgonController::HandleChasingState(float delta)
{
    MoveTowardsTarget(delta);
    // glm::vec2 currentVelocity = m_pVelocity->GetVelocity();
    // printf("After MoveTowardsTarget - Velocity: (%f, %f)\n", currentVelocity.x, currentVelocity.y);

    const glm::vec2 targetPosition = m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
    const glm::vec2 currentPosition = m_pTransform->GetGlobalPosition();
    const float distanceToTarget = glm::length(targetPosition - currentPosition);

    // If player is out of detection range or within detection range but already petrified, switch to prospect
    if 
    (
        distanceToTarget > m_detectionRange ||
        (distanceToTarget <= m_detectionRange && m_pTargetStatusComponent != nullptr && m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    )
    {
        ChangeState(EnemyState::PROSPECT);
    }

    if (!m_transitionTimer.IsRunning())
    {
        m_transitionTimer.Start();
    }

    // If target is within ranged range
    if (distanceToTarget <= m_rangedRange)
    {
        if 
        (
            m_pTargetStatusComponent != nullptr                                                             &&  // If target status component not null
            m_transitionTimer.Elapsed() >= m_transitionDelay                                                &&  // If transition delay expired
            !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED)   &&  // If target not already petrified
            IsTargetInLOS()                                                                                     // If target in line of sight
        )
        {
            ChangeState(EnemyState::ATTACKING);
        }
    }
}

void GorgonController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;
    // Stop Gorgon's movement during attack
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    if(m_rangedTimer <= 0.0f)
    {

        // If target is in line of sight, petrify target and switch to prospect
        if(m_pTargetStatusComponent != nullptr && IsTargetInLOS())
        {
            m_pTargetStatusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 5.0f);
        }
        ChangeState(EnemyState::CHASING);
    }
    else
    {
        // Cooldown timer for next attack
        m_rangedTimer -= delta;

        // Brighten sprite to indicate attack
        if(m_pAnimComponent != nullptr)
        {
            m_pAnimComponent->SetTint(m_pAnimComponent->GetTint() + delta / (m_rangedCooldown * 0.5f));
        }
    }
}

void GorgonController::HandlePetrifiedState(float delta)
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::PETRIFIED);
    m_pAnimComponent->SetAnimPaused(true);
    m_pVelocity->SetVelocity(glm::vec2(0.0f, 0.0f));    
}

void GorgonController::HandleStunnedState(float delta)
{
    if(m_stunnedTimer >= m_stunnedTime)
    {
        ChangeState(EnemyState::PROSPECT);
    }
    else
    {
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
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
            ItemDropCreator::Instance()->CreateItemDropFromLootTable("data/minitaur_loot.yaml", m_pTransform->GetGlobalPosition(), -1.0f);
            GetGameObject()->Delete();
        }
        m_lieDeadTimer += delta;
    }
}

void GorgonController::EnterIdleState()
{
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

}

void GorgonController::EnterChasingState()
{
    m_transitionTimer.Reset();
}

void GorgonController::EnterStunnedState()
{
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);
}

void GorgonController::ExitAttackState()
{
    m_rangedTimer = m_rangedCooldown;
    m_stunnedTimer = 0.0f;
    
    if(m_pAnimComponent != nullptr)
    {
        m_pAnimComponent->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
    }
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
    m_targetDetectionTimer = m_RNG.NextFloat(0.4f, 0.8f);
    m_prospectCounter = 0;
    m_prospectStandingCounter = m_RNG.NextFloat(0.5f, 2.0f);

}

void GorgonController::ExitStunnedState()
{
    
    m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
    m_stunnedTimer = 0.0f;
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

    // Get labyrinth manager
    LabyrinthManager* lbmg = nullptr;
    for (auto&& [_, labyrinthManager] : GetGameObject()->GetScene().Each<LabyrinthManager>())
    {
        lbmg = &labyrinthManager;
        break;
    }

    // Check
    if(lbmg != nullptr)
    {
        glm::vec2 thisPos = m_pTransform->GetGlobalPosition();
        glm::ivec2 thisTilePos = lbmg->GetTilePosition(thisPos);
        int thisTileID = lbmg->GetTile(thisTilePos.x, thisTilePos.y);

        glm::vec2 targetPos = this->m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition();
        glm::ivec2 targetTilePos = lbmg->GetTilePosition(targetPos);
        int targetTileID = lbmg->GetTile(targetTilePos.x, targetTilePos.y);

        // If either this tile or target tile is invalid, return false
        if(thisTileID < 0 || targetTileID < 0)
        {
            printf("Gorgon Controller - ERROR: INVALID TILE\n");
            return false;
        }

        // If either entity or target is inside wall (somehow), return false
        if(this->IsWallTile(thisTileID) || this->IsWallTile(targetTileID))
        {
            return false;
        }

        if(thisTilePos == targetTilePos && !this->IsWallTile(thisTileID))
        {
            return true;
        }

        const int tileSize = (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE);

        glm::vec2 line = targetPos - thisPos;
        glm::vec2 normalisedLine = glm::normalize(line);

        glm::ivec2 currentTilePos = thisTilePos;
        int currentTileID = thisTileID;
        glm::vec2 rayLength = glm::vec2(0.0f, 0.0f);
        glm::ivec2 tileStep = glm::vec2(0, 0);
        glm::vec2 rayStep = glm::vec2(
            sqrt(1 + pow((normalisedLine.y / normalisedLine.x), 2)),
            sqrt(1 + pow((normalisedLine.x / normalisedLine.y), 2))
        );
        // std::cout << "GorgonController - Normalised Line - x: " << normalisedLine.x << ", y: " << normalisedLine.y << std::endl;
        // std::cout << "GorgonController - This Tile Pos - x: " << thisTilePos.x << ", y: " << thisTilePos.y << std::endl;
        // std::cout << "GorgonController - Target Tile Pos - x: " << targetTilePos.x << ", y: " << targetTilePos.y << std::endl;

        if(line.x > 0.0f)
        {
            tileStep.x = 1;
            rayLength.x = (this->GetTileWorldPos(glm::ivec2(thisTilePos.x + 1, thisTilePos.y)).x - thisPos.x) * rayStep.x;
        }
        else
        {
            tileStep.x = -1;
            rayLength.x = (thisPos.x - this->GetTileWorldPos(glm::ivec2(thisTilePos.x - 1, thisTilePos.y)).x) * rayStep.x;
        }

        if(line.y > 0.0f)
        {
            tileStep.y = 1;
            rayLength.y = (this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y + 1)).y - thisPos.y) * rayStep.y;
        }
        else
        {
            tileStep.y = -1;
            rayLength.y = (thisPos.y - this->GetTileWorldPos(glm::ivec2(thisTilePos.x, thisTilePos.y + 1)).y) * rayStep.y;
        }
        
        // Iterate until target tile is reached
        bool isTargetTileReached = false;
        while(true)
        {
            if(
                (currentTilePos.x == targetTilePos.x) && 
                (currentTilePos.y == targetTilePos.y)
            )
            {
                return true;
            }

            // std::cout << "GorgonController - Ray Length - x: " << abs(rayLength.x) << ", y: " << abs(rayLength.y) << std::endl;
            if(abs(rayLength.x) < abs(rayLength.y))
            {
                currentTilePos.x += tileStep.x;
                rayLength.x += rayStep.x;
            }
            else
            {
                currentTilePos.y += tileStep.y;
                rayLength.y += rayStep.y;
            }

            currentTileID = lbmg->GetTile(currentTilePos.x, currentTilePos.y);
            // If tile is a wall, return false
            if(this->IsWallTile(currentTileID))
            {
                return false;
            }
        }
    }

    return false;
}

bool GorgonController::IsWallTile(int p_tile_id)
{
    return (p_tile_id >= Tile::WallBottomLeft) && (p_tile_id <= Tile::WallTop);
}

glm::vec2 GorgonController::GetTileWorldPos(glm::ivec2 p_tile_pos)
{
    glm::vec2 res = glm::vec2(
        p_tile_pos.x * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE), 
        p_tile_pos.y * (LabyrinthManager::SCALE * LabyrinthManager::TILE_SIZE)
    );
    return res;
}