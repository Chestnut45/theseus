#include "GorgonController.h"
#include "PlayerController.h"
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
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
        // On exiting petrified state
        if(m_state == EnemyState::PETRIFIED)
        {
            m_state = EnemyState::IDLE;
            m_pAnimComponent->SetAnimPaused(false);
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
            HandleIdleState();
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

void GorgonController::HandleIdleState()
{
    // Check if the player is within detection range
    float distanceToPlayer = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());

    // If the player comes into detection range and not petrified, start chasing
    if (distanceToPlayer <= m_detectionRange && m_pTargetStatusComponent != nullptr && !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::CHASING); 
    }
}

void GorgonController::HandleProspectState(float delta)
{
    // Chase player if in range and not petrified
    float distanceToPlayer = glm::length(m_pTarget->GetComponent<wolf::Transform2D>()->GetGlobalPosition() - m_pTransform->GetGlobalPosition());
    if (distanceToPlayer <= m_detectionRange && m_pTargetStatusComponent != nullptr && !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
    {
        ChangeState(EnemyState::CHASING); 
        m_prospectCounter = 0;
    }

    // else, prospect
    else
    {
        if (m_prospectStandingCounter <= 0.0f)
        {
            if(m_prospectCounter <= 0)
            {        
                    
                    // Roll for prospect
                    float rng = m_RNG.NextInt(1, 100);
                    // Begin prospecting
                    if(rng > 10)
                    {
                        
                        m_prospectCounter = m_RNG.NextInt(1, 3);
                        glm::vec2 direction = glm::normalize(glm::vec2(m_RNG.NextInt(-100, 100), m_RNG.NextInt(-100, 100)));
                        m_pVelocity->SetVelocity(direction * m_chaseSpeed);
                    }

                    // Change to idle
                    else
                    {
                        ChangeState(EnemyState::IDLE);
                        m_pVelocity->SetVelocity(glm::vec2(0.0f)); // Reset velocity when returning to idle
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
        return;
    }

    if (!m_transitionTimer.IsRunning())
    {
        m_transitionTimer.Start();
    }

    // If target is within ranged range
    if (distanceToTarget <= m_rangedRange)
    {
        // If transition delay is expired and target is not already petrified, attack
        if (m_transitionTimer.Elapsed() >= m_transitionDelay && m_pTargetStatusComponent != nullptr && !m_pTargetStatusComponent->IsStatusEffectActive(StatusComponent::StatusEffectType::PETRIFIED))
        {
            ChangeState(EnemyState::ATTACKING);
            m_transitionTimer.Reset();
        }
    }
    // If target is out of range, set timer to 0 & set transition delay to random value
    else
    {
        m_transitionTimer.Reset();
        m_transitionDelay = m_RNG.NextFloat(0.8f, 1.6f);
    }
}

void GorgonController::HandleAttackingState(float delta)
{
    if (!m_pTarget) return;

    // Stop Gorgon's movement during attack
    m_pVelocity->SetVelocity(glm::vec2(0.0f));

    if(m_rangedTimer <= 0.0f)
    {
        //---------------------------//
        //                           //
        //  TODO: ADD HITSCAN CHECK  //
        //                           //
        //---------------------------//
        if(true)
        {
            // Petrify target and switch to prospect
            if(m_pTargetStatusComponent != nullptr)
            {
                m_pTargetStatusComponent->AddStatusEffect(StatusComponent::StatusEffectType::PETRIFIED, 5.0f);
                ChangeState(EnemyState::PROSPECT);
                AnimatedSprite2D* sprite = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
                if(sprite != nullptr)
                {
                    sprite->SetTint(glm::vec3(1.0f, 1.0f, 1.0f));
                }
            }
        }
        
        
        // Reset attack cooldown timer
        m_rangedTimer = m_rangedCooldown;        
    }
    else
    {
        // Cooldown timer for next attack
        m_rangedTimer -= delta;

        // Brighten sprite to indicate attack
        AnimatedSprite2D* sprite = this->GetGameObject()->GetComponent<AnimatedSprite2D>();
        if(sprite != nullptr)
        {
            sprite->SetTint(sprite->GetTint() + delta / (m_rangedCooldown * 0.5f));
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
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::NONE);
        
        m_stunnedTimer = 0.0f;
        ChangeState(EnemyState::PROSPECT);
    }
    else
    {
        m_pAnimComponent->SetSpecialEffects(AnimatedSprite2D::SpecialEffectsType::WHITE);

    }
    m_stunnedTimer += delta;
}

void GorgonController::UpdateAnimationBasedOnDirection()
{
    if (!m_pAnimComponent || !m_pVelocity) return;

    std::string animationName;

    // Get the current velocity to determine direction
    glm::vec2 velocity = m_pVelocity->GetVelocity();

    // Only update animation if the Gorgon is moving
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

void GorgonController::ChangeState(EnemyState newState)
{
    m_state = newState;
}